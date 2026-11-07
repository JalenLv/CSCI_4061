# Multi-threaded HTTP server in Python (Project 3, Part 2): Claude's implementation

A complete Part 2. The three files the spec marks `EDIT` hold all of the work:
`http_handler.py`, `http_server.py` and `connection_queue.py`. The `Makefile`
gained the extra-test targets below (`make test` is unchanged). Everything else
is the unmodified starter.

## Status

| Suite | Result |
| --- | --- |
| Provided tests (`make test`) | 2/2 |
| Extra tests (`make test-extra`) | 15/15 |

## Running it

From the repo root, in the course container:

```sh
IMG=$(docker images --format '{{.Repository}}:{{.Tag}}' | grep '^vsc-csci_4061' | head -1)
docker run --rm -i --ulimit nofile=1024:1024 --cap-add SYS_PTRACE --security-opt seccomp=unconfined \
  -v "$PWD":/work -w /work/Projs/proj3/claudes-impl/part2 "$IMG" bash -lc 'make test-all'
```

Inside a shell: `make test` (ports 8000, 8001), `make test-extra` (ports
8051-8065, about 30 s), `make test-extra testnum=10`. Don't run `make test`
here right after Part 1's on the same ports: see Part 1's "Test environment
quirks". The top-level `make test-all` keeps them apart.

The extra tests are Part 1's, run against `python3 http_server.py`, minus the
C-only ones (TSAN, valgrind) and with `queue_test.py` in place of
`queue_test.c`. [Part 1's NOTES](../part1/NOTES.md#extra-tests) describes
them; the numbering here is 1–4 as there, 5 the queue, then 6–15 = Part 1's
7–16.

## How it works

- **`read_request`** wraps the socket in `conn.makefile("rb")` and reads lines
  until `b"\r\n"`. The request line's second word is the resource; anything
  but `GET /...` raises `ValueError`, and EOF before the empty line raises
  `ConnectionError`. Closing the file object leaves the socket open.
- **`write_response`** uses `pathlib`, as the spec suggests: anything that
  isn't `is_file()` gets the 404. Otherwise it opens the file *before* sending
  the header (so a failure can't leave a 200 with no body), sends status line,
  `Content-Type` and `Content-Length`, then `conn.sendfile(file)`.
- **`ConnectionQueue`**: one `Lock` shared by two `Condition`s, `not_full` and
  `not_empty`. The same rules as Part 1: `while` loops around `wait()`,
  `notify()` on every enqueue and dequeue, `notify_all()` on both at shutdown,
  and dequeue keeps returning elements after shutdown until the deque is
  empty, then raises `ShutDownException`.
- **`http_server.py`**: `signal.signal(SIGINT, handle_sigint)`, then
  `socket.create_server(("", port))` (binds `0.0.0.0`, sets `SO_REUSEADDR`),
  `settimeout(1)`, 5 worker threads, and an accept loop that retries on
  `TimeoutError` and enqueues each connection. Shutdown closes the server
  socket, shuts the queue down, joins the workers and exits with
  `sys.exit(exit_code)`.
- **Workers** loop on `dequeue()` until `ShutDownException`. Each connection
  is handled inside `with conn:`, and any `OSError` (the client hung up:
  `BrokenPipeError`, `ConnectionResetError`) or `ValueError` (a bad request)
  is printed to stderr. That one client is lost; the thread lives on.

## Design choices not dictated by the spec

1. **The starter's MIME table is kept as shipped**, including `".jpg":
   "text/jpeg"`, where the standard type is `image/jpeg` (Part 1's
   `get_mime_type()` says `image/jpeg`). A hidden test that checks types was
   presumably made with the instructor's own table, which the starter's most
   likely copies; the provided tests don't check types. Extra test 1 expects
   the table's values. If you change `.jpg` to `image/jpeg`, its two `.jpg` lines
   differ; that's a difference, not a bug.
2. **`pathlib` is imported in `http_handler.py`.** The spec's two "no other
   libraries" rules name `connection_queue.py` and `http_server.py`, and it
   points to `pathlib` for this file.
3. **Dequeue drains after shutdown**, as in Part 1. The starter's comments
   only say a shut-down queue raises `ShutDownException`; the spec says the
   queue works "the same way as your C code", and the C rules let dequeues
   continue until the queue is empty. Extra tests 5, 10 and 11 assume it.
4. **Workers catch `OSError` and `ValueError` per client.** The spec says to
   stop a worker on `ShutDownException`; an uncaught exception from a client
   that hung up would end the thread just the same and shrink the pool
   silently. See P1 below.
5. **The server socket is closed before the workers are joined**, as in
   Part 1.

## Python signal handling, measured

- **The handler isn't late; `accept()` is restarted.** A scratch copy that
  timestamps the handler, sent SIGINT 1.5 s after starting, three runs each:
  the handler ran at once (0.00 s after the signal), in `MainThread`, every
  time. But the accept loop only ended when the current `accept()` timed out:
  3.52 s later with a 5 s timeout, 0.52 s later with 1 s. PEP 475: CPython
  runs the handler when `accept()` fails with `EINTR`, then retries the call
  with the time remaining. Without a timeout the retry blocks until the next
  client connects (P3 below). The spec's wording, that the handler waits for
  the system call to return, describes the effect, not the mechanism.
- **Shutdown latency** is therefore up to one timeout period. Extra test 9
  allows 3 s.

### Classic mistakes, and which tests catch them

Copies with one mistake each, run through both suites twice on the final
test files, as in Part 1. The unmodified control passed everything both
times.

| Variant | Mistake | Provided | Extra failures | What you see |
| --- | --- | --- | --- | --- |
| P1 | No `try`/`except` in the worker | **2/2** | 13 | Each abandoned download kills a worker. The five requests afterwards wait at the `concurrent_open` barrier: `no EOF within 10 s`. Everything else passes, and the server still exits 0 |
| P2 | One `conn.recv(1024)` per request | **2/2** | 3, 4 | `malformed request line b'G'`; long requests are answered, then `connection reset` |
| P3 | No `settimeout()` | 0/2 | all but 5, 11 | `server still running 5 s after SIGINT; killed`. 11 passes: main was blocked in enqueue, not `accept()` |
| P4 | `notify()` instead of `notify_all()` in `shut_down` | 0/2 | all but 10, 11 | The join never returns; 3 queue unit tests fail |
| P5 | Dequeue raises as soon as the queue is shut down | **2/2** | 5, 10, 11 | Queued clients get `connection reset` |
| P6 | No `with conn:` (never closed explicitly) | **2/2** | 1–4, 6, 12–15 | `no EOF within 10 s`: the socket closes only when the worker drops its reference at its next `dequeue()`. `curl` doesn't notice |
| P7 | A thread per connection | **2/2** | 6 | `threads before any client: 1`. Shutdown still serves everyone, because Python waits for non-daemon threads at exit |
| P8 | `if` instead of `while` around `wait()` in dequeue | **2/2**, 1/2 once | 5, 6 | Both stress runs fail: a woken consumer finds the deque empty and raises `ShutDownException` before any `shut_down()`. In the server that worker quits: 5 threads after 20 concurrent requests. Once, the pool shrank during provided test 2 and its barrier never filled (`Empty reply from server`) |
| P9 | `notify()` only when the deque was empty | **2/2** | 8 (timeout) | Lost wakeups, as Part 1's V3: workers sleep with connections queued, and the 25-request barrier test never completes |

The bold ones pass both provided tests.

## Oral-exam Q&A

- **What does `makefile()` buy over `recv()`?** Buffering: `readline()`
  returns exactly one line however the bytes arrived, where one `recv(1024)`
  returns whatever has arrived so far (P2).
- **Why `sendfile`?** It hands the copying to the kernel (`os.sendfile`, no
  trip through Python buffers) and loops until the whole file is sent.
- **Why `with conn:`?** It closes the socket however the handler exits.
  Without it the socket is only closed when the worker drops its last
  reference, at its next `dequeue()`, and an HTTP/1.0 client waiting for EOF
  waits that long (P6).
- **Why no signal blocking in workers, as in Part 1?** It's possible
  (`signal.pthread_sigmask`), but Python always runs handlers in the main
  thread, and the measurement above found the signal delivered there anyway.
  What blocking can't fix is PEP 475's retry, hence the timeout.
- **Lock vs Condition?** A `Condition` wraps a lock and adds `wait()`, which
  releases the lock while sleeping and reacquires it before returning, plus
  `notify()`/`notify_all()`. Two conditions on one lock is the same pattern
  as Part 1's mutex and two condition variables.
- **What happens to a socket left in the deque at exit?** With drain
  semantics, nothing is left: the workers empty the queue before they return.
