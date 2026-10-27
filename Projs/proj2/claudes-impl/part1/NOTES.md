# swish pipelines (Project 2, Part 1): Claude's implementation

A complete implementation of Part 1. The only file the assignment marks `EDIT`,
`swish_funcs.c`, holds all of the work. The `Makefile` gained the extra-test
targets below (`make test` itself is unchanged). Everything else
(`swish.c`, `string_vector.[ch]`, `swish_funcs.h`, the provided `.o` files) is
the unmodified starter code.

## Status

| Suite | Result |
| --- | --- |
| Provided tests (`make test`) | 8/8 pass |
| Extra tests I wrote (`make test-extra`) | 20/20 pass |
| Valgrind under testius (26 shell runs + the 7 forked children that never exec'd), plus extra test 19's own 6 reports | 0 errors, 0 bytes in use at exit, in every process |
| `clang-format --dry-run --Werror` (starter style) | Clean |

## Running it

Everything runs inside the course container, as root (the image's default). From
the repo root:

```sh
IMG=$(docker images --format '{{.Repository}}:{{.Tag}}' | grep '^vsc-csci_4061' | head -1)
docker run --rm -i --ulimit nofile=1024:1024 --cap-add SYS_PTRACE --security-opt seccomp=unconfined \
  -v "$PWD":/work -w /work/Projs/proj2/claudes-impl/part1 "$IMG" bash -lc 'make && make test-all'
```

For a shell inside (to drive `./swish` by hand), use `-it` and end the command
with plain `bash`. Then, inside:

```sh
make                          # build swish
make test                     # the 8 provided tests
make test-extra               # my 20 extra tests (20-30 s; test 18 alone 11-14 s, on a 16-core host)
make test-extra testnum=13    # one extra test, verbose
make test-all                 # both suites
./swish                       # drive it by hand
grep -h "ERROR SUMMARY\|in use at exit" test_results/*valgrd.tmp   # see "Valgrind" below
```

**Running the extra tests against your own code.** Copy your `part1/` into a
scratch directory, copy this directory's `test_cases/extra/` to
`test_cases/extra/` in the copy, and run `testius test_cases/extra/tests_extra.json`
there. The copy must include `swish_funcs_provided_x86_64.o` (the starter's
prebuilt object, which the link step needs; your `code/part1/` was missing it
until 10-25). Test 13 is the only test that checks the
empty-command message. The spec defines none, so it uses your wording
(`Error: Empty command`); any other wording is a difference, not a bug.

**Valgrind.** testius only fails a test when the *shell* exits with valgrind's
error code. A forked child that never exec's (command not found, failed
redirect) writes its own leak report into the same `*-valgrd.tmp` file, and a
leak there does not fail the test. A file with more than one `ERROR SUMMARY`
line holds child reports; read all of them. Extra test 19 automates this for
five such children: it runs valgrind itself with one log per process and fails
if any report shows errors or memory in use at exit.

## How it works

**Parsing.** `has_empty_command()` rejects a leading `|`, a trailing `|` or two
adjacent `|` before anything is created. Then `n_pipes =
strvec_num_occurrences(tokens, "|")` and `n_cmds = n_pipes + 1`. The commands
are cut out from the end of the line backwards: `end` starts at
`tokens->length`, `find_pipe_before(tokens, end)` returns the `|` before it (or
-1 for the first command), `strvec_slice(tokens, &cmd, bar + 1, end)` copies the
command, and `end = bar` moves on to the previous command.

**Pipe array.** One `malloc`'d `int pipes[2 * n_pipes]`, all created in the
parent before the first `fork()`. Pipe `i` connects command `i` to command
`i + 1`, so command `i` reads from `pipes[2*(i-1)]` (none for the first) and
writes to `pipes[2*i + 1]` (none for the last). For `a | b | c | d`:

| Command | `in_idx` (stdin from) | `out_idx` (stdout to) |
| --- | --- | --- |
| 0 `a` | -1 (terminal or `<` file) | 1 (pipe 0, write end) |
| 1 `b` | 0 (pipe 0, read end) | 3 (pipe 1, write end) |
| 2 `c` | 2 (pipe 1, read end) | 5 (pipe 2, write end) |
| 3 `d` | 4 (pipe 2, read end) | -1 (terminal or `>`/`>>` file) |

**Launch order.** Last command first, as the spec recommends. A `pid_t` array
records each child so the parent can `waitpid()` exactly those children.

**Descriptor discipline** (the whole assignment, really):

1. Child, in `run_piped_command()`: `dup2()` the read end onto stdin and the
   write end onto stdout, then close **all** `n_pipe_fds` originals, including
   the two just duplicated, then `run_command()`.
2. Parent, after the launch loop: close all `2 * n_pipes` ends, **then**
   `waitpid()` every child.

A reader sees EOF only when every write end of its pipe, in every process, is
closed. The parent holds a copy of every write end, so it must close them
before it waits; each child holds copies of every write end too, so each child
must close the ones it doesn't use.

**Error paths.** Every call that can fail is checked and reported with
`perror("<function>")`. What each path cleans up:

| Failure | Reported as | Cleanup |
| --- | --- | --- |
| Empty command | `Error: Empty command` (stderr) | Nothing created yet |
| `fflush(stdout)` | `fflush: ...` | Nothing created yet |
| `malloc` (pipes, pids) | `malloc: ...` | Frees whichever array exists |
| `pipe()` number *k* | `pipe: ...` | Closes the 2*k* ends already made, frees both arrays |
| `strvec_slice()` | `strvec_slice: Cannot allocate memory` | Clears the partial slice, then the normal close-all and reap of children already started |
| `fork()` | `fork: ...` | Same as above |
| `close()` in the parent | `close: ...` | Keeps closing the rest, still reaps every child, returns -1 |
| `waitpid()` | `waitpid: ...` | Keeps reaping the rest, returns -1 |
| Child: `dup2()` | `dup2: ...` | Closes every pipe end, then the child frees its slice, its copy of `tokens`, `pipes` and `pids`, and calls `exit(1)` |
| Child: `close()` | `close: ...` | Same, after trying to close the rest |
| Child: `run_command()` returns (exec or `open` failed) | `exec: ...` or `Failed to open ... file: ...` (printed by `run_command`) | Same child cleanup |

`strvec_slice()` doesn't set `errno` itself, but it can only fail when `malloc`
or `realloc` does, and those set `ENOMEM`. Calling `strvec_clear()` on a
failed slice is always safe: `strvec_slice()` initializes `dest` first, and if
that very `malloc` failed, `strvec_clear()` frees `NULL` with length 0.

The return value is -1 only when one of the shell's own calls fails. A command
that exits non-zero (including a child that couldn't exec) is reaped but does
not change the result, which is how bash treats a pipeline's members.
`swish.c` ignores the return value anyway.

### Error paths, tested by fault injection

The hard-to-trigger paths were tested by linking a scratch build with
`-Wl,--wrap=fork,--wrap=pipe,--wrap=dup2` and a small shim whose `__wrap_fork()`
etc. fail on the *n*-th call (counted per process, chosen by an environment
variable) and otherwise call `__real_fork()`. All runs were under valgrind with
testius's options, stdin and stdout redirected to files:

| Injected failure | Input | Output | Valgrind |
| --- | --- | --- | --- |
| 3rd `fork()` | `seq 1 5 \| cat \| cat \| wc -l`, then `ls -l /proc/self/fd \| grep -c pipe:` | `fork: Resource temporarily unavailable`, `0` (the already-running `wc` got EOF), then `1` (no leaked pipe ends) | 1 process, clean |
| 2nd `pipe()` | `seq 1 5 \| cat \| wc -l`, then the same check | `pipe: Too many open files`, then `1` | 1 process, clean |
| every child's 1st `dup2()` | `seq 1 5 \| cat \| wc -l` | `dup2: Bad file descriptor` three times | 4 processes (shell + 3 children), all clean |

## Design choices not dictated by the spec

1. **Empty commands are refused before anything is created.** Without the
   check (variant V8 below), the provided `run_command()` receives an empty
   vector and the child segfaults inside it. Valgrind logs `SIGSEGV` in each
   such child, and `| wc -l` prints `0`. The shell itself survives, because
   only the child dies. The message is the owner's wording; the spec defines none.
2. **`fflush(stdout)` before forking.** The prompt is printed without a newline.
   When stdout is not a terminal it is fully buffered, so `@> ` is still in the
   buffer at `fork()` time and every child inherits a copy. A child that execs
   discards it, but a child whose exec fails calls `exit()`, which flushes it.
   Measured with stdout piped (`printf ... | ./swish | cat -A`) and the
   `fflush` removed: `nosuchcmd | wc -c` printed `3` (the prompt went down the
   pipe into `wc`), and stdout held five prompts instead of three. With the
   flush it prints `0`. Tests that drive swish through testius's pty can't see
   this: there stdout is line-buffered and flushed whenever the shell reads
   stdin. Extra test 20 pipes swish's stdout through `cat -A` instead, and
   without the flush it prints `3$ x$ @> @> @>` rather than `@> 0$ @> x$ @>`.
3. **`waitpid()` on recorded pids** rather than calling `wait()` `n` times, so
   the shell can never reap a child that isn't part of this pipeline. That
   would matter once this is combined with Project 1's background jobs.
4. **`close_fds()` keeps going after a failed `close()`** so that one bad
   descriptor doesn't leak the others.
5. **A small `find_pipe_before()` helper instead of `strvec_find_last()` +
   `strvec_take()`.** The new helpers suggest walking a working copy of the
   vector and chopping off the last command each time. That works, but it
   needs a second vector that every child and every error path must also free.
   Scanning backwards from an index keeps `tokens` read-only.

## Learning route

Build it in this order, and run the checkpoint before moving on.

1. **Parsing only.** Count the pipes, and temporarily print each command's
   `[start, end)` from the parent without forking. Checkpoint:
   `a b | c | d e f` gives `[5, 8) [3, 4) [0, 2)` when walking backwards, and
   `| a`, `a |` and `a | | b` are each refused.
2. **Two commands, one pipe.** Checkpoint: provided test 1 (`echo hello | cat`).
   If it hangs, a write end is still open somewhere. Look at the parent first,
   then at the child that reads.
3. **The general case.** Use the index formula from the table above. Checkpoint:
   extra tests 1–4. Test 4 (ten `sed`s, each appending one letter) shows a
   dropped or reordered stage immediately.
4. **Redirection.** There is nothing to write. `run_command()` already handles
   `<`, `>` and `>>`. Checkpoint: provided tests 2–4 and 6–8, extra tests 5–8.
5. **Failure paths.** Make the child free everything before `exit()`. Checkpoint:
   extra tests 9–13 and 19 (which counts the failed children's leak reports),
   then read the `*-valgrd.tmp` files of tests 9–12. Each should hold one clean
   summary per child that failed, as well as the shell's. Then test 20
   (`fflush` before forking).
6. **Hygiene and volume.** Checkpoint: extra tests 14–18.

### Classic mistakes, and which tests catch them

Each row is a one-change variant of this implementation, run against both suites.
V1 and V3 hang on everything, so they skipped test 18 (120 s timeout) and show
results out of 19.

| Variant | Mistake | Provided | Extra | What you see |
| --- | --- | --- | --- | --- |
| V1 | Parent waits, *then* closes its pipe ends | 0/8 | 0/19 | Every test times out: no reader ever sees EOF |
| V2 | Parent closes only the write ends | **8/8** | 17/20 (16, 17, 18) | Children inherit leaked read ends: counts of 3, 5, then 8 instead of 2, 2, 1. Test 18 hits `pipe: Too many open files` at about pipeline 1008 |
| V3 | Child closes nothing after `dup2` | 0/8 | 0/19 | Every test times out. The last command holds a write end of its own input |
| V4 | Child closes everything except the two it duplicated ("forgets the dup'd fds") | **8/8** | 17/20 (16, 17, 18) | No hang, just leaks: the exec'd program has 1–2 extra pipe descriptors |
| V5 | Child closes everything, *then* calls `dup2` | 0/8 | 0/20 | `dup2: Bad file descriptor` from every child |
| V6 | Launch first to last and wait for each command before starting the next | **8/8** | 16/20 (14, 15, 17, 19) | Works for small outputs (≤ 64 KiB fits in the pipe), times out when a writer fills the pipe before its reader exists. (It fails 19 only because this hand-written variant's child also skips its frees.) |
| V7 | No `fflush(stdout)` before forking | **8/8** | 19/20 (20) | Only visible when stdout isn't a terminal: test 20 prints `3$ x$ @> @> @>`; see design choice 2 |
| V8 | No empty-command check | **8/8** | 19/20 (13) | Children segfault inside `run_command()` |
| V9 | No `waitpid()` | 4/8 | 3/20 | The next prompt appears before the output. After `seq 1 3 \| cat \| wc -l`, `ps --ppid` shows `wc`, `cat` and `seq` as `Z <defunct>` |
| V10 | Child whose command fails calls `exit(1)` without freeing | **8/8** | 19/20 (19) | Test 19 prints `valgrind reports: 6, with errors: 5, with memory in use at exit: 5` (144–203 bytes per child). testius's own valgrind check never sees it |

V2, V4, V6, V7, V8 and V10 pass every provided test. Those are the bugs the
hidden tests, the error-checking review or the oral exam would find.

**`head` and SIGPIPE.** `seq 1 100000 | head -n 1`: `head` exits after one
line, and `seq`'s next write hits a pipe with no reader. Launched from bash,
`seq` dies silently from SIGPIPE and you see `1`. Under testius you also see
`seq: write error: Broken pipe`, because testius runs the shell with SIGPIPE
ignored (see "Test environment quirks"), so `write()` fails with `EPIPE`
instead. Both runs were measured with this implementation. It's not a shell
bug, and the spec promises no test does this.

## Oral-exam Q&A

- **Why `n - 1` pipes?** A pipe joins two neighbors. `n` commands have `n - 1`
  neighboring pairs. The first command's stdin and the last command's stdout
  stay as they are (terminal or file).
- **Why must the parent close its pipe ends before waiting?** `read()` on an
  empty pipe returns 0 (EOF) only once *no* process holds a write end. The
  parent created every pipe, so it holds every write end. If it waits first,
  the last reader blocks forever, never exits, and the wait never returns
  (V1: everything times out).
- **Why must each child close the descriptors it doesn't use?** Same reason:
  after `fork()` every child holds every end. The last command would hold a
  write end of its own input pipe and wait for itself forever (V3). The two
  ends it *does* use are now stdin/stdout, so the originals are just extra
  references. Leaving them open doesn't hang anything, but the exec'd program
  inherits descriptors it should never see (V4, extra test 16).
- **What does `dup2()` do to the open file description?** `dup2(old, new)`
  closes `new` if it was open, then makes `new` refer to the *same* open file
  description as `old`, adding one reference. Closing `old` afterwards removes
  one reference, and the pipe stays open through `new`. In V4 `ls -l
  /proc/self/fd` showed fd 1 and fd 4 as the same `pipe:[216670027]`. The pipe
  end is really closed only when its last reference, in any process, is closed.
- **Why launch backwards?** Correctness doesn't depend on the order, because
  every pipe exists before the first `fork()` and the parent waits only after
  launching everything. Backwards is what the spec recommends, and it pairs
  naturally with finding each command's start from the `|` before it. One
  practical difference: if a `fork()` fails midway, the commands already
  running are downstream readers, which get a clean EOF once the parent
  closes its ends. Measured: `wc` printed `0` in the fork-failure run above.
  Launched forwards, the survivors would be writers, which would get SIGPIPE
  or `EPIPE` instead. That last part is reasoning, not something I ran.
- **What does `run_command()` do after our `dup2`?** It scans for `<`, `>`
  and `>>`. For each one it `open()`s the file, `dup2()`s it onto stdin or
  stdout and closes the original, then cuts the argument list and calls
  `execvp()`. (Measured: in test 16's `ls -l /proc/self/fd > out.txt`,
  `out.txt` appears only as fd 1, and `sort -rn < file` gets no `<` in its
  arguments.) On failure it prints `Failed to open input file`, `Failed to open
  output file` or `exec` via `perror()` and returns -1. It does not exit.
- **`|` vs `<`/`>`: which wins?** The file, because `run_command()`'s `dup2`
  runs after ours. The spec only allows `<` on the first command and `>`/`>>`
  on the last, where there is no pipe on that side anyway, so the question only
  arises outside the spec. Measured: `true | cat < numbers.txt | wc -l` prints
  30 (`cat` read the file), and `seq 1 5 > out.txt | wc -l` prints 0 with
  `out.txt` holding 1–5.
- **How are zombies avoided?** Every pid that `fork()` returned is
  `waitpid()`'d, including after a `fork()` failure partway through. V9 (no
  waits) left `seq`, `cat` and `wc` as `<defunct>` children of the shell. The
  reference leaves none.
- **Pipe buffer size and blocking?** 65536 bytes on this kernel
  (`F_GETPIPE_SZ`; a non-blocking writer filled exactly 65536 bytes before
  `EAGAIN`). `write()` blocks while the pipe is full. `read()` blocks while it
  is empty and a writer exists, and returns 0 when it is empty and no writer
  exists. Writing with no reader left raises SIGPIPE, or returns `EPIPE` if
  SIGPIPE is ignored. This is why waiting for each command in turn (V6) works
  for small outputs and deadlocks above 64 KiB: the writer blocks on a full
  pipe whose reader hasn't been started yet.
- **Why does a child whose command fails `exit()` instead of returning?** It is
  a copy of the shell. Returning would put a second shell in the read loop on
  the same stdin. Before exiting it frees its copies of the shell's heap (the
  slice, `tokens`, `pipes`, `pids`), so its valgrind report is clean too
  (extra test 19 checks this; V10 above shows what skipping it looks like).
- **Why check `close()`?** It can fail (`EBADF` for a bookkeeping bug, `EIO`),
  and the rubric asks for every failure-returning call to be checked. A close
  failure is reported and the remaining descriptors are still closed.

## Extra tests

All in `test_cases/extra/`, manifest `tests_extra.json`. Tests 1–18 run under
the same valgrind setting as the provided tests. Test 19 runs valgrind itself
(`resources/child_leaks.sh`, one log per process), and test 20 runs swish
without valgrind with its stdout piped. Inputs avoid writers whose reader exits early (testius
ignores SIGPIPE, so those would print a racy `Broken pipe`), which is why some
use `true` or `cat /dev/null` as the first command.

| # | What it checks |
| --- | --- |
| 1 | 4 commands: distinct values in `numbers.txt` (24) |
| 2 | 5 commands where every stage changes the data |
| 3 | The spec's 7-command word-frequency pipeline (`tr \| tr \| sort \| uniq -c \| sort -n \| tail`) on `resources/words.txt` |
| 4 | 10 commands; each `sed` appends one letter, so the output shows order and count |
| 5 | `<` on the first of 4 commands (`cat <` and `sort -rn <`) |
| 6 | `>` on the last of 5 commands, file checked afterwards |
| 7 | `<` first and `>` last on the 7-command pipeline |
| 8 | `>` then `>>` from two 4-command pipelines into one file |
| 9–11 | Command not found as the first, middle and last command: `exec: No such file or directory`, the other members still run, the shell continues |
| 12 | `<` from a missing file and `>` into a missing directory inside pipelines |
| 13 | Leading, trailing, doubled and lone `\|` are refused, and the shell continues |
| 14 | About 1.3 MB through one and then three pipes (well past 64 KiB) |
| 15 | `sort` can only print after EOF on 588 KB of input |
| 16 | Pipe descriptors a child inherits, as the first, middle and last command (1, 2, 2, 1) |
| 17 | After normal, exec-failure, redirect-failure and large pipelines, a child still inherits only its own pipe (no empty-command line, so the message wording doesn't matter here) |
| 18 | 1100 pipelines in one session (one leaked descriptor per line exhausts the 1024 limit), then the inherited-pipe check |
| 19 | Five children that can't start (missing program first, middle and last; bad `<` file; bad `>` path) all free their memory: `valgrind reports: 6, with errors: 0, with memory in use at exit: 0` |
| 20 | stdout piped through `cat -A`: no duplicated prompt (`fflush` before forking) |

Every expected output was written as a prediction before running. One was
wrong: test 15 first expected `100000 99999 99998`, but `sort -rn | tail -n 3`
keeps the three *smallest* values (3, 2, 1). That was my arithmetic, not the
shell.

## Test environment quirks (measured)

- **testius runs the shell with SIGPIPE ignored.** `cat /proc/self/status |
  grep SigIgn` under testius printed `0000000001001000` (SIGPIPE and SIGXFSZ),
  and `0000000000000000` when swish was started from bash. Python ignores
  SIGPIPE, and `pty.fork()` + `exec` keeps that setting, which every child
  inherits. Any writer whose reader exits early therefore prints `Broken pipe`
  under testius instead of dying silently.
- **Valgrind's log file is open in every exec'd child.** Under testius
  (`--log-file=...`), `ls /proc/self/fd | wc -l` prints 5, not 4: fd 3 is the
  `*-valgrd.tmp` file. Without valgrind, or with valgrind logging to stderr,
  it prints 4. That's why tests 16–18 count `pipe:` entries from `ls -l
  /proc/self/fd` instead of all entries. (The pattern is `pipe:` rather than
  `pipe` because the log file is named after the suite. Under the provided
  suite's name, "Swish Shell Pipeline Tests", fd 3's line would match `pipe`
  too.)
- **What testius compares.** The pty's output, so stdout and stderr together,
  with blank lines dropped and runs of whitespace collapsed (`uniq -c`'s
  padding doesn't matter).
