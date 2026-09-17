# swish — Claude's implementation

A complete implementation of CSCI 4061 Project 1. Only the two files the
assignment marks `EDIT` were changed: `swish.c` and `swish_funcs.c`. Everything
else (`Makefile`, `string_vector.[ch]`, `job_list.[ch]`, `swish_funcs.h`) is the
unmodified starter code.

## Status

| Suite | Result |
| --- | --- |
| Provided tests (`make test`) | 32/32 pass |
| Extra tests I wrote (`test_cases/extra/`) | 17/17 pass |
| Valgrind (all 49 runs, incl. forked children) | 0 errors, 0 bytes in use at exit |
| `clang-format` | Matches the starter files' style byte-for-byte |

## Running it

Everything below runs inside the course devcontainer image. From the repo root:

```sh
docker run --rm -it \
  --ulimit nofile=1024:1024 --cap-add SYS_PTRACE --security-opt seccomp=unconfined \
  -v "$PWD/Projs/proj1/claudes-impl":/work -w /work \
  vsc-csci_4061-<hash>:latest bash
```

Then, inside:

```sh
make                # build swish and slow_write
make test           # run the 32 provided tests
make test testnum=5 # run just one
./swish             # drive it by hand

testius test_cases/extra/test_extra.json          # my 17 extra tests
testius test_cases/extra/test_extra.json -v -n 17 # verbose, single test
```

Note: extra test 1 (`cd` with no argument) compares against `$HOME`, so it only
proves anything if `$HOME` differs from the working directory. I ran the suite
with `-e HOME=/tmp`.

## What each task does

**Task 0 — `tokenize()`** (`swish_funcs.c`). `strtok(s, " ")` once, then
`strtok(NULL, " ")` in a loop, pushing each token with `strvec_add()`.

**Task 1 — `pwd` / `cd`** (`swish.c`). `getcwd()` into a `CMD_LEN` buffer;
`chdir()` to token 1, or to `getenv("HOME")` when token 1 is absent. Both report
failure with `perror()` and re-prompt rather than exiting.

**Task 2 — `run_command()`**. Copies token pointers into a `char *args[]` array
terminated by `NULL`, then `execvp()` so `PATH` is searched. It only returns if
`exec` failed, and it prints `perror("exec")` itself — `main()` deliberately
prints nothing extra on that path, which is what test 12 checks.

**Task 3 — redirection**. `strvec_find()` locates `<`, `>`, `>>`. Because the
assignment guarantees redirection tokens come after all program arguments, the
argument list simply ends at the *lowest* index among the operators found — that
is what `arg_count` computes. Two helpers, `redirect_input()` and
`redirect_output()`, do the `open()` + `dup2()` + `close()`; `redirect_output()`
takes an `append` flag that picks `O_APPEND` over `O_TRUNC`, so `>` and `>>`
share one code path.

**Task 4 — process groups**. The child resets `SIGTTIN`/`SIGTTOU` to `SIG_DFL`
and calls `setpgid(pid, pid)` before touching anything else. The parent calls
`tcsetpgrp()` before waiting and again (with its own pid) after.

**Task 5 — stopped jobs**. `waitpid(child_pid, &status, WUNTRACED)`; on
`WIFSTOPPED(status)` the child goes into the job list as `STOPPED`.
`resume_job()` looks the job up, hands it the terminal, `kill(pid, SIGCONT)`,
waits the same way, and removes the job if it did not stop again.

**Task 6 — background jobs**. A trailing `&` is detected with `strcmp` on the
last token and removed with `strvec_take()`. The background path skips both
`tcsetpgrp()` and `waitpid()` and records the job as `BACKGROUND`.
`await_background_job()` refuses a `STOPPED` job (waiting on one would hang the
shell forever). `await_all_background_jobs()` waits on every `BACKGROUND` entry,
flips any that stopped to `STOPPED`, and then clears the rest in one
`job_list_remove_by_status(jobs, BACKGROUND)` — the removal happens after the
walk because removing nodes mid-iteration would invalidate the cursor.

## Three things I did that the assignment doesn't ask for

These are all in service of the "Error Checking" rubric or of not printing
spurious errors. Each is small and independent — delete any of them and all 49
tests still pass.

1. **The parent also calls `setpgid(child_pid, child_pid)`** (`swish.c`). The
   assignment puts `setpgid()` only in the child, but the parent then races
   ahead to `tcsetpgrp(STDIN_FILENO, child_pid)`. If the parent wins that race,
   the process group doesn't exist yet and `tcsetpgrp()` fails with `ESRCH`.
   Calling `setpgid()` from both sides is what real shells do; whichever runs
   first wins and the second call is a harmless no-op. `EACCES` (child already
   `exec`'d) and `ESRCH` (child already exited) are tolerated rather than
   reported, since both mean the group is already sorted out.

2. **`tcsetpgrp()` is guarded by `isatty(STDIN_FILENO)`** (`swish.c` and
   `resume_job()`). With a pty this changes nothing, so every test behaves
   identically. Without one — `printf "pwd\nexit\n" | ./swish`, which is how
   you'll often poke at it while debugging — the unguarded version prints
   `tcsetpgrp: Inappropriate ioctl for device` after *every* command, because
   a pipe has no foreground process group to set.

3. **`fflush(stdout)` before `fork()`** (`swish.c`). The prompt is printed
   without a trailing newline, so it can still be sitting in stdout's buffer at
   `fork()` time when stdout isn't line-buffered. The child would then inherit a
   copy and flush it on exit, printing a second prompt. glibc happens to flush
   line-buffered streams when you read from stdin, which hides this on a
   terminal, but the explicit flush makes it not depend on that.

## Oral-exam material: things worth being able to explain

- **Why `WUNTRACED`?** Without it `waitpid()` only returns when the child
  *terminates*, so Ctrl-Z would leave the shell blocked forever on a process
  that is merely paused.
- **Why does the child restore `SIGTTIN`/`SIGTTOU` to `SIG_DFL`?** Children
  inherit handlers across `fork()`. The shell ignores those two so it survives
  being moved to the background, but a user program that reads from the terminal
  while backgrounded is *supposed* to stop, which is the default action.
- **Why give the child its own process group at all?** Terminal-generated
  signals (Ctrl-C → `SIGINT`, Ctrl-Z → `SIGTSTP`) go to every process in the
  terminal's foreground process group. If the child stayed in the shell's group,
  Ctrl-C would kill the shell too.
- **Why must the shell take the terminal back after waiting?** Otherwise the
  dead child's group stays foreground, and the shell — now a background process
  reading from the terminal — would get `SIGTTIN`. That's exactly the signal it
  ignores, so instead of stopping, its `read()` would fail with `EIO`.
- **Why can't `run_command()` return on success?** It ends in `execvp()`, which
  replaces the process image. Anything after it only runs if `exec` failed.
- **Why does the failing child `return 1` instead of continuing the loop?** It
  is a duplicate of the shell; letting it fall back into the read loop would
  give you two shells fighting over one terminal.
- **`>` vs `>>`** is only the difference between `O_TRUNC` and `O_APPEND` in the
  `open()` flags; both use `O_WRONLY | O_CREAT` with mode `S_IRUSR | S_IWUSR`.
- **Why remove terminated jobs only after the `await_all_background_jobs()`
  loop?** `job_list_remove()` frees the node, which would leave the loop's
  `current->next` pointing into freed memory.

## Extra tests

`test_cases/extra/` covers the gaps the assignment description calls out plus a
few others:

| # | What it checks |
| --- | --- |
| 1 | `cd` with no argument goes to `$HOME` |
| 2–4 | `>>` appends, creates the file, and differs from `>` |
| 5 | Redirected and plain commands interleaved |
| 6 | Four different failures in a row; shell keeps prompting |
| 7–8 | Background jobs with output and input redirection |
| 9 | `fg` on a job that is *running* in the background, not stopped |
| 10 | A background job and a suspended job alive at once |
| 11 | Suspend → `bg` → `wait-all` |
| 12 | `wait-all` skips stopped jobs and leaves them in the list |
| 13 | Repeated Ctrl-C / Ctrl-Z across several programs |
| 14 | `fg`/`bg`/`wait-for` with missing or negative indices |
| 15 | Two background jobs awaited individually, out of order |
| 16 | Relative and absolute `cd` interleaved with programs |
| 17 | One long session using every feature together |

## Known simplifications (all permitted by the spec)

- One `<` and at most one of `>`/`>>` per command; operators must follow the
  program's arguments. The spec grants all three assumptions.
- A `bg` on a job that reads the terminal will be stopped again immediately by
  `SIGTTIN` while the job list still says `background`. The spec explicitly says
  not to handle a background job being re-suspended.
- Background jobs that finish are not reaped until `wait-for`, `wait-all`, or
  `fg` asks about them, so they sit as zombies until then. A real shell uses a
  `SIGCHLD` handler; this project's design deliberately doesn't.
