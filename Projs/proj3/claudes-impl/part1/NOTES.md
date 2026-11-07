# Multi-threaded HTTP server (Project 3, Part 1): Claude's implementation

A complete Part 1. The four files the spec marks `EDIT` hold all of the work:
`http.c`, `http_server.c`, `connection_queue.h` and `connection_queue.c`. The
`Makefile` gained the extra-test targets below (`make test` is unchanged).
Everything else is the unmodified starter.

## Status

| Suite | Result |
| --- | --- |
| Provided tests (`make test`) | 2/2 |
| Extra tests (`make test-extra`) | 18/18 |
| Valgrind, testius's options (extra 17: concurrent, missing and misbehaving clients, then SIGINT) | 0 errors, all heap blocks freed |
| ThreadSanitizer: the server (extra 18) and the queue unit tests (extra 6) | 0 warnings |
| Fault injection (failing `pthread_create`, `accept`, `open`), under valgrind | each reported, clean exit, 0 errors |
| `clang-format` (the starter's style) | clean |

## Running it

From the repo root, in the course container (root, as always):

```sh
IMG=$(docker images --format '{{.Repository}}:{{.Tag}}' | grep '^vsc-csci_4061' | head -1)
docker run --rm -i --ulimit nofile=1024:1024 --cap-add SYS_PTRACE --security-opt seccomp=unconfined \
  -v "$PWD":/work -w /work/Projs/proj3/claudes-impl/part1 "$IMG" bash -lc 'make && make test-all'
```

Inside a shell (`-it`, ending the command with plain `bash`):

```sh
make                          # http_server, http_server_tsan, concurrent_open.so
make test                     # the 2 provided tests (ports 8000, 8001)
make test-extra               # the 18 extra tests (ports 8011-8028), about 30 s
make test-extra testnum=11    # one extra test, verbose
./http_server server_files 4061 & sleep 0.5; curl -s localhost:4061/quote.txt; kill -INT %1
```

Every extra test that starts a server binds its own port, `PORT_BASE + n`
(`PORT_BASE` = `port_base + 10`). A port can't be bound again for about a
minute after its connections close (see "Test environment quirks").

## How it works

### Reading a request (`read_http_request`)

It `read()`s up to 512 bytes at a time until it has seen the empty line that
ends a GET request, `\r\n\r\n`, however the client split the request into
packets. The marker is matched byte by byte with a counter (how much of the
marker the bytes so far end with), so it is found even when it straddles two
reads (extra test 4). The first line is copied out as it goes by. It must be
shorter than 512 bytes, so the resource name inside it is too; that is the
size `http_server.c` allocates. `strtok_r()` (re-entrant, as the spec asks)
splits it, and anything other than `GET /...` is refused.

It reads the whole request even though only the first line matters, as the
spec says. Closing a socket with unread data makes the kernel send RST instead
of a normal FIN. V1 below shows it: each response arrived whole, then the
client saw `connection reset` instead of a clean end of stream.

### Writing a response (`write_http_response`)

1. `stat()`. `ENOENT`, or `ENOTDIR` for a path like `/quote.txt/more`, means
   404, and so does anything that isn't a regular file (a directory). Other
   `stat()` errors are reported and the connection closed.
2. `open()` before sending a single byte, so that a failure can't leave the
   client with a `200` status and no body.
3. The MIME type comes from the text after the last `.` of the file name (not
   of the whole path), through the starter's `get_mime_type()`. No extension,
   or an unknown one, gets `application/octet-stream`.
4. The status line and both headers go out in one `snprintf()` buffer, then
   the file in 512-byte chunks. `write_all()` loops because `write()` may write
   less than asked.

The 404 is the spec's exact `HTTP/1.0 404 Not Found\r\nContent-Length: 0\r\n\r\n`.

### The queue

`connection_queue_t` gains one mutex and two condition variables, `not_full`
and `not_empty`, over the starter's circular buffer (`read_idx`, `write_idx`,
`length`). Every `pthread_*` call is checked. They return an error number
rather than setting `errno`, so errors are printed with `strerror()`, as
`concurrent_open.c` does.

- **enqueue** waits on `not_full` *in a loop* while the queue is full and not
  shut down, fails if it was shut down, adds the fd and signals `not_empty`.
- **dequeue** waits on `not_empty` in a loop while the queue is empty and not
  shut down. After a shutdown it keeps returning elements until the queue is
  empty, then returns -1.
- **shutdown** sets the flag and *broadcasts* both conditions, under the lock.
- **free** destroys the three primitives, reporting each failure.

### The server

1. `sigaction(SIGINT)` with `sa_flags = 0`: no `SA_RESTART`, so a blocked
   `accept()` fails with `EINTR` instead of being restarted.
2. `getaddrinfo(NULL, port)` with `AI_PASSIVE`, `SOCK_STREAM`, `AF_UNSPEC`,
   then `socket()`, `bind()`, `freeaddrinfo()`, `listen(5)`. In the container
   the first result is `0.0.0.0`, and `curl localhost` connects to
   `127.0.0.1` (both measured).
3. Block every signal with `sigprocmask()`, saving the old mask; create the 5
   workers; restore the mask. Measured in `/proc/<pid>/task/*/status`: each
   worker's `SigBlk` is `fffffffe7ffbfeff` (everything except SIGKILL, SIGSTOP
   and glibc's two internal signals 32 and 33), and main's is 0.
4. The accept loop runs while `keep_going`. `EINTR` goes back to the loop
   condition. Any other `accept()` error is reported and ends the loop with
   exit status 1. Each new fd is enqueued; the main thread never talks to a
   client.
5. Shutdown: close the listening socket, shut the queue down, join the
   workers (each finishes its client, then drains the queue), free the queue,
   exit 0.

Each worker loops on `dequeue()`, serves one client, and closes the fd, until
`dequeue()` returns -1.

### Error paths, by fault injection

Linked with `-Wl,--wrap=pthread_create,--wrap=accept,--wrap=open` and a shim
that fails the n-th call, all under valgrind with testius's options:

| Injected failure | What happened | Valgrind |
| --- | --- | --- |
| 3rd `pthread_create` (`EAGAIN`) | `pthread_create: Resource temporarily unavailable`, the 2 started workers joined, exit 1 | clean |
| 2nd `accept` (`EMFILE`) | first client served, `accept: Too many open files`, exit 1 | clean |
| 1st `open` (`EACCES`) | `open: Permission denied`, that client gets the connection closed with no response, the next one is served, SIGINT exits 0 | clean |
| port `notaport` | `getaddrinfo failed: Servname not supported for ai_socktype`, exit 1 | |

## Design choices not dictated by the spec

1. **Dequeue drains after shutdown.** The header comment says a shut-down
   queue makes dequeue fail at once; the spec says "threads can continue to
   perform a dequeue operation until the queue becomes empty". This follows
   the spec. It also means no accepted connection is dropped at shutdown:
   extra tests 11 and 12 send SIGINT with clients queued, and all are served.
2. **Signal `not_empty` on every enqueue**, not only when the queue was
   empty. The spec describes the effect ("if this operation adds a new element
   to what was an empty queue, one thread waiting to dequeue is unblocked"),
   but signalling only on that transition loses wakeups: two enqueues can land
   before the first woken worker runs, and the second signal never happens.
   See V3 below.
3. **The listening socket is closed before the workers are joined**, so a
   client that connects during shutdown is refused rather than left in the
   backlog until the process exits.
4. **Directories and `ENOTDIR` paths get 404**, and unknown extensions get
   `application/octet-stream`. The spec only has 200 and 404, and requires a
   `Content-Type` on every 200.
5. **A malformed or unfinished request is reported on stderr and the
   connection closed without a response**, since the spec defines no 400.
   Extra test 13 accepts any behavior that closes the connection.
6. **Any `accept()` error other than `EINTR` shuts the server down** (status
   1) rather than retrying, which could spin on a persistent error like
   `EMFILE`.
7. **`keep_going` is `volatile sig_atomic_t`**, the type C guarantees a signal
   handler can write safely. The starter's `int` works at `-O0` as built.
8. **No `SO_REUSEADDR`.** The spec treats the linger period as something to
   work around with `port_base`, and the extra tests use a fresh port per run.
9. **`..` in a path is not rejected.** Measured: `curl --path-as-is
   localhost:9001/../http.h` returns the server's own `http.h`. The spec
   doesn't ask, and it's a good oral-exam question (see Q&A).

## Learning route

1. **Sequential server first** (Task 1): socket setup, a loop that accepts,
   reads, writes and closes, all in `main`. Checkpoint: provided test 1, then
   extra tests 1–4 against it. They are single-client tests, so the
   sequential version should pass them. Test 4 needs the byte-by-byte marker
   match.
2. **The queue, on its own.** Checkpoint: extra tests 5 and 6 (unit tests,
   then the same under ThreadSanitizer). They need nothing from the server.
3. **The pool.** Create workers with signals blocked, enqueue in the accept
   loop. Checkpoint: provided test 2, extra tests 7–9.
4. **Shutdown.** Checkpoint: extra tests 10–12. Test 12 is the main thread
   blocked in a full enqueue when SIGINT arrives.
5. **Robustness and hygiene.** Checkpoint: extra tests 13–16.
6. **Tools.** Extra tests 17 (valgrind) and 18 (TSAN), then the TSAN binary by
   hand under `LD_PRELOAD`.

### Classic mistakes, and which tests catch them

Each row is a copy of this implementation with one mistake, run through both
suites twice on the final test files, in fresh containers four at a time. The
unmodified copy ran alongside as a control and passed everything both times.
"Once" means in one of the two rounds.

| Variant | Mistake | Provided | Extra failures | What you see |
| --- | --- | --- | --- | --- |
| V1 | One `read()` per request | **2/2** | 3, 4 | 3: the server gets `G`, calls it malformed and closes; the client's next send hits `Broken pipe`. 4: every response arrives whole, then `connection reset`: the server closed with request bytes unread |
| V2 | Loop on `read()`, `strstr(chunk, "\r\n\r\n")` per chunk | **2/2** | 3, 4 (timeouts) | A marker split across two reads is never seen, so the server waits for input that never comes |
| V3 | Signal `not_empty` only when the queue was empty | **2/2** | 9 (timeout) | Workers sleep while connections sit in the queue, so the barrier never sees 5 opens at once. Provided test 2's 10 requests got through in all 4 rounds this variant ran (two earlier, on older test files); extra 9's 25 never did |
| V4 | `if` instead of `while` around `pthread_cond_wait` in dequeue | **2/2** | 5, 6; 7 once | The stress run with 1 producer and 8 consumers: `FAIL: dequeue returned -1 before the queue was shut down` (also 10 of 10 standalone runs, 5 of 5 under TSAN). A woken worker that finds the queue empty takes this dequeue's "shut down and empty" exit and quits, so test 7 counts 5 or 4 threads after 20 concurrent requests |
| V5 | Shutdown broadcasts `not_empty` only | **2/2** | 5, 6 | Only the unit tests: `FAIL: shutdown wakes both blocked enqueuers`. In the server the only enqueuer is main, which is also the thread shutting down |
| V6 | Shutdown *signals* `not_empty` | 0/2 | all but 11, 12 | `server still running 5 s after SIGINT; killed`: one worker wakes, `pthread_join` waits forever on the others. 11 and 12 pass because every worker was busy at SIGINT; a worker that comes back to an empty, shut-down queue never waits |
| V7 | Dequeue fails as soon as the queue is shut down | **2/2** | 5, 6, 11, 12 | Queued clients get `no complete header in 0 bytes (connection reset)` at shutdown |
| V8 | Workers don't block signals | **2/2** | 7 | `other threads blocking SIGINT: 0 of 5`. Nothing else noticed: in every run SIGINT reached main anyway, and SIGPIPE never arises unless the server keeps writing (V16) |
| V9 | `SA_RESTART` | 0/2 | all but 5, 6, 12 | SIGINT is ignored until the next connection. 12 passes: at SIGINT main was in enqueue, not `accept()` |
| V10 | Worker never closes the client socket | **2/2** | 1–4, 7, 13–18 | `no EOF within 10 s`. `curl` passes (it stops after `Content-Length` bytes), and so do the shutdown tests (exit closes everything) |
| V11 | `write_http_response` never closes the file | **2/2** | 15 | `open: Too many open files` at request 1111, with 1018 descriptors leaked |
| V12 | 404 without `Content-Length` | **2/2** | 2, 17, 18 | `HTTP/1.0 404 Not Found \| body 0 bytes` |
| V13 | Headers end in `\n`, not `\r\n` | **2/2** | 1–4, 7, 11–16 | `curl` accepts it; the raw client finds no `\r\n\r\n`: `no complete header in 129 bytes` |
| V14 | A detached thread per connection, no pool | **2/2** | 7, 11, 12 | `threads before any client: 1`. At SIGINT main returns and exit kills requests in flight: `no complete header in 0 bytes (closed)`. Valgrind still passes |
| V15 | Workers also test `keep_going` in their loop | **2/2** | 11, 12 (timeout), 18 | 18: `ThreadSanitizer warnings: 2`, exit status 66 (a data race with the handler's write). 11: workers quit with clients still queued. 12: deadlock, main blocked in a full enqueue that no worker will ever empty |
| V16 | Body loop ignores `write_all()`'s failure, and workers don't block signals | **2/2** | 7, 14 | 14: the second failed `write()` raises SIGPIPE, nothing blocks it, and the process dies: `server exit status: 141`, then `could not connect` |
| V17 | Body loop ignores `write_all()`'s failure; signal mask as the spec says | **2/2** | none | The same ignored errors, but SIGPIPE stays pending on the blocked worker and each `write()` just fails with `EPIPE`. The spec's signal mask is the difference from V16 |

Every variant passes provided test 1 except those that never exit on SIGINT
(see "Test environment quirks"). The ones in bold pass both provided tests:
those are the bugs only hidden tests, the error-checking review or the oral
exam would find.

## Oral-exam Q&A

- **Why no `SA_RESTART`?** With it, the kernel restarts `accept()` after the
  handler returns, the loop never re-checks `keep_going`, and the server only
  notices SIGINT after the next client connects (V9).
- **Why block signals before `pthread_create`?** A thread inherits its
  creator's mask. Blocking everything first means the workers start with a
  full mask, so the kernel can only deliver SIGINT to main, the one thread in
  `accept()`. Restoring the old mask afterwards lets main receive it.
- **`sigprocmask` or `pthread_sigmask`?** POSIX leaves `sigprocmask` in a
  multi-threaded process unspecified; on Linux both change the calling
  thread's mask. The first call happens before any thread exists; the spec
  names `sigprocmask`.
- **What if SIGINT arrives while main is blocked in a full enqueue?**
  `pthread_cond_wait()` doesn't return on a signal. The handler sets
  `keep_going` and main stays in enqueue until a worker frees a slot. Then it
  enqueues, sees `keep_going == 0` and shuts down (extra test 12).
- **Is there a window where SIGINT is missed?** Yes, between the loop's
  `keep_going` test and the `accept()` call. A signal landing there runs the
  handler, and `accept()` then blocks until the next connection. It is
  nanoseconds wide; the real fixes are `ppoll()`/`pselect()` with the signal
  unblocked atomically, `signalfd()`, or a self-pipe. This is reasoning; I
  didn't reproduce it.
- **Why `while`, not `if`, around `pthread_cond_wait`?** A woken thread
  reacquires the mutex later, and by then another thread may have taken the
  element (or freed slot). Wakeups can also be spurious. In V4 such a worker
  found the queue empty, returned -1 as if shut down, and quit: the pool
  shrank to 4 or 5 threads under 20 concurrent requests.
- **Why broadcast on shutdown?** Every blocked worker must wake up and see the
  flag. `pthread_cond_signal` wakes one, so `pthread_join` waits forever on the
  others (V6).
- **Why hold the mutex while signalling?** It's the course rule. What
  actually prevents a lost wakeup is changing the predicate under the mutex
  that the waiter holds while testing it; signalling while still holding it
  keeps that easy to see.
- **What does `listen(sock, 5)`'s 5 mean?** The backlog: connections the
  kernel completes and holds until `accept()`. Beyond it, new SYNs are dropped
  and the client retries. Measured with 40 simultaneous `curl`s: 20 connected
  at once, 17 after 1.01 s, 3 after 2.04 s, and all 40 finished within 2.1 s.
  That's the main thread blocking in a full enqueue while the backlog fills.
- **What does `concurrent_open.so` check?** It wraps `open()` and friends.
  Each `open()` of a path containing `server_files/` waits until 5 such calls
  are pending at once, then all 5 proceed. Fewer than 5 workers means waiting
  forever: a timeout. It also means the opens must come in fives: a server
  that opened something extra under `server_files/` would leave the last ones
  waiting.
- **Why doesn't a client hanging up kill the server with SIGPIPE?** Two
  reasons, both measured. After the client's kernel sends RST, the server's
  first failed `write()` returns `ECONNRESET`; only a later `write()` on that
  socket raises SIGPIPE. `write_all()` stops at the first error, so in 20
  aborted downloads SIGPIPE was never generated (every log line was `write:
  Connection reset by peer`, and no thread had a pending signal). And workers
  block every signal, so a SIGPIPE would only stay pending on that thread,
  with `write()` returning `EPIPE`. V16 and V17 show both halves: keep writing
  after the error with signals unblocked and SIGPIPE kills the server (exit
  status 141); keep writing with the spec's mask and it survives.
- **Why `Content-Length`?** HTTP/1.0 bodies otherwise end at EOF, and clients
  use the length to know the body is complete. `curl` stops reading after that
  many bytes.
- **Why read the request to the end?** See "Reading a request": closing with
  unread input sends RST.
- **Why `strtok_r`?** `strtok` keeps its position in a hidden static
  variable, shared by every thread.
- **What does `AI_PASSIVE` do?** With a NULL host it returns the wildcard
  address (`0.0.0.0` or `::`), so the server accepts connections on every
  interface.
- **What does `GET /../http.h` do?** Here it serves the server's own source,
  because the path is just appended to `server_files` (measured with `curl
  --path-as-is`; plain `curl` normalizes the `..` away first). A real server
  canonicalizes the path, with `realpath()` and a prefix check, before
  opening it.
- **What does TSAN do?** It instruments every memory access and
  synchronization call and reports two accesses to the same location, at
  least one a write, with no happens-before order between them.

## Extra tests

In `test_cases/extra/`, manifest `tests_extra.json`. Each test runs one script
from `resources/` with the server command and its test number (the server
binds `PORT_BASE` plus that number), and the script prints a short
deterministic summary. `resources/http_client.py` is a raw-socket
client: it shows the status line and headers as sent, checks CRLF line endings
and the body byte for byte, and notes whether the server closed the
connection. Server output is only checked in runs where a correct server has
nothing to say.

| # | What it checks |
| --- | --- |
| 1 | Status line, `Content-Type`, `Content-Length` and body for all 11 files |
| 2 | 404 with `Content-Length: 0` for a missing file, a missing directory, a path below a file, a name without extension |
| 3 | Request sent 1 byte, then 7 bytes, at a time |
| 4 | `\r\n\r\n` straddling 512-, 1024-, 4096- and 8192-byte read boundaries; 64 KiB of headers |
| 5, 6 | Queue unit tests (`queue_test.c`): FIFO with wraparound, blocking when full and empty, shutdown wakes everyone, drain after shutdown, and two stress runs (4 producers with 4 consumers; 1 producer with 8 mostly waiting consumers, where a dequeue that returns -1 before shutdown fails it); 6 is the same under TSAN |
| 7 | 6 threads (main + 5) before any client, after 100 sequential and 20 concurrent requests; SIGINT blocked in all 5 workers, not in main |
| 8 | 40 concurrent downloads, all intact |
| 9 | 25 downloads under `concurrent_open.so`: five rounds that each need all 5 workers |
| 10 | SIGINT when idle: exits within 3 s, status 0, silent |
| 11 | SIGINT with 5 slow clients in the workers and 3 in the queue: all 8 served |
| 12 | SIGINT with main blocked in a full enqueue: all 11 served |
| 13 | A client that sends nothing, one that sends half a request, one malformed: connections closed, server keeps serving |
| 14 | 5 clients abandon a 32 MiB download, server started with SIGPIPE at its default action; 5 concurrent requests under `concurrent_open.so` afterwards |
| 15 | 1500 sequential requests against the 1024-descriptor limit; descriptor count unchanged |
| 16 | A second server on a taken port exits nonzero, mentioning "Address already in use" |
| 17 | Valgrind: 0 errors, all heap blocks freed |
| 18 | `http_server_tsan`: 0 ThreadSanitizer warnings, exit 0 |

Every expected output was written as a prediction before its first run. The
first 18 all matched. One later check didn't: a deterministic "stolen wakeup"
unit test assumed the woken thread usually loses the race for the mutex, and
under TSAN (and in Python) it won every time, so the mostly-waiting stress run
replaced it. Test scripts take the test number, not a port: testius treats
output ending in `$` as its prompt, so a `$` in an input line can make it type
the next line early when the pty echo is split there. That happened 3 times in
52 suite runs before the change.

## Test environment quirks (measured)

- **Ports linger after the server closes connections.** The server closes
  each connection first, so its end goes into TIME_WAIT (3 requests left 3
  TIME_WAIT entries on the server's port in `/proc/net/tcp`). Rebinding that
  port then fails for about a minute with `bind: Address already in use`.
  Python's `socket.create_server()` sets `SO_REUSEADDR`, and Linux only allows
  the reuse when the TIME_WAIT sockets had it too. Measured: C after C, Python
  after C and C after Python all fail; Python after Python works. That's why
  every extra test gets its own port, and why running Part 2's tests right
  after Part 1's on the same ports fails.
- **The top-level `make test` runs Part 2 on ports 3 and 4.** Its
  `py_port_base` adds 3 to `primary_port`, which that Makefile never defines.
  It works anyway: the container runs as root, and Docker sets
  `ip_unprivileged_port_start` to 0 inside it (the host's is 1024), so even a
  normal user can bind port 80 there. The spec's "permission denied" for ports
  below 1024 only applies outside the container. `claudes-impl/Makefile`'s
  added targets give each suite its own ports instead.
- **Provided test 1 can fail spuriously under load.** Its input lines contain
  `$PORT1`, and testius takes output ending in `$` as its prompt: when the pty
  echo splits right after that `$`, testius types `pkill` before the 404 line
  arrives, and the output is out of order. Seen 4 times in 141 provided-suite
  runs, four containers at a time, always with code that doesn't touch that
  path. Rerun before suspecting the code.
- **Provided test 1 also fails when the server ignores SIGINT.** Its output
  still matches, but the server it started in the background keeps the pty
  open, so testius waits until its timeout (V6, V9, P3, P4 below).
- **testius runs everything with SIGPIPE ignored** (Python ignores it, and
  `pty.fork()` plus `exec` keeps that). Extra test 14 restores the default
  action with `resources/sigpipe_default.py`, as a terminal launch would have.
