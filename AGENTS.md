# CSCI 4061 (Fall 2026) — agent guide

Coursework for UMN CSCI 4061, Introduction to Operating Systems: C systems
programming on Linux (processes, I/O, signals). Everything here is graded work
the owner writes themselves. That shapes most of the rules below.

## Ground rules

- **The owner does their own coursework.** Explain concepts, answer questions,
  review code, run tests and report results. Do not edit their solutions
  (lab `code/*.c`, `QUESTIONS.txt`, `Projs/*/code/`) unless asked to. If you spot
  a bug, describe it and let them fix it. Reading their files is fine.
- **Agent-owned directories** are the exception: when asked for a reference
  implementation, it goes in its own directory next to `code/` (for example
  `Projs/proj1/claudes-impl/`), and agents may edit it freely.
- Don't commit or push unless asked. Commit from the host, not from inside the
  devcontainer (see Pitfalls).
- `HWs/` is out of scope for now.

## Layout

```
.devcontainer/          Course Docker image (Ubuntu 22.04, gcc, gdb, valgrind, testius, socrates)
Labs/labNN/labNN.pdf    Lab slides
Labs/labNN/code/        Unpacked starter zip, completed in place
Projs/projN/desc/       Spec, converted from the Canvas page to projectN.md, plus figures
Projs/projN/code/       Unpacked starter zip, completed in place (the owner's submission)
Projs/projN/<name>/     Agent-owned reference implementation, if one was requested
```

The history follows a fixed pattern: `Init labNN` or `Init projN` commits the
starter code and spec, and `Complete labNN` commits the finished work.

## Building and testing

The owner builds and tests in the course devcontainer. The tests are only
guaranteed to behave the same there. The image is already built locally. From
the repo root:

```sh
IMG=$(docker images --format '{{.Repository}}:{{.Tag}}' | grep '^vsc-csci_4061' | head -1)
docker run --rm -i --ulimit nofile=1024:1024 --cap-add SYS_PTRACE \
  --security-opt seccomp=unconfined --user "$(id -u):$(id -g)" -e HOME=/tmp \
  -v "$PWD":/work -w /work/<path/to/code> "$IMG" bash -lc 'make && make test'
```

- Use `-it` instead of `-i` to drive a program interactively.
- `--user` keeps build output owned by the host user. Keep it, and don't switch
  to `--user root` on the real mount: that is how root-owned files end up in
  `code/` (see Pitfalls). If `code/` already holds root-owned output from a
  devcontainer run, a `--user` run there fails, because testius can't empty the
  root-owned `test_results/`. Copy the sources, `Makefile` and `test_cases/`
  into a scratch directory and test there instead. `--user root` is harmless
  against a scratch copy.
- `HOME=/tmp` gives that uid a usable home directory, since the image's
  passwd file doesn't list it. Some tests `cd` to `$HOME`.
- POSIX man pages are installed in the image: `man 2 fork`, `man 7 signal`.

**testius** is the course test runner. Each `code/` dir has a JSON manifest
under `test_cases/`.

- `make test` runs everything. Most Makefiles accept `make test testnum=N`; the
  runner itself takes `-v -n N`.
- Test input files can contain `^C`, `^Z` and `^D` lines. testius runs the
  program in its own pty and sends those as real keystrokes.
- In expected output, `{{cmd}}` is replaced by the output of running `cmd` in
  a shell (for example `{{pwd}}`).
- Results go in `test_results/`. `*-results.tmp` has a side-by-side diff, and
  when the manifest sets `use_valgrind`, `*-valgrd.tmp` has the valgrind log.
- testius **empties `test_results/`** at the start of each run.

**Extra tests from a reference implementation.** An agent-owned reference
implementation (see Ground rules) may come with its own extra tests, for example
`Projs/proj1/claudes-impl/test_cases/extra/`. They're written to the spec, so
they also stand in for hidden tests against the owner's code. The owner's
`code/` has no target for them, and agents don't add files there. Instead, make
a fresh scratch copy of it (see the `--user` note above), copy the reference's
`test_cases/extra/` into the copy's `test_cases/`, and run testius on that
manifest in the copy. These tests can check strings the spec never defines, or
features the owner hasn't written yet. Check Pitfalls for known cases before
reporting a failure as a bug.

Build flags are always `-Wall -Werror -g`, so any warning fails the build.

**Code style.** There's no `.clang-format` in the repo. The starter code
matches `{BasedOnStyle: LLVM, IndentWidth: 4, ColumnLimit: 100,
SpacesBeforeTrailingComments: 4, SpaceAfterCStyleCast: true}` (clang-format 19
is in the image).

## Labs

Each lab has a QUIZ part and a CODE part. It's submitted with `make zip`, and
the zip is uploaded to Gradescope.

- **QUIZ:** mark answers in `QUESTIONS.txt` by changing `( )` to `(X)`. Change
  nothing else in that file. `socrates` checks the answers against a hash in
  `test_cases/resources/quiz_sum.json`, so the correct answers can't be read
  from the repo. `QUESTIONS.txt.bak` is the pristine copy.
- **CODE:** complete the `.c` files the lab names.
- **Targets:** `make test-quiz`, `make test-code`, `make test`, `make zip`. The
  targets differ slightly between labs, so check `make help`.

| Lab | Topic | Files |
|---|---|---|
| lab01 | `fork()`, `exec()`, `wait()` | `fork_wait.c`, `fork_exec.c` |
| lab02 | Signals, job control, file descriptors, `dup2()` redirection | `print_nums.c`, `redirect_child.c` |
| lab03 | Catching SIGINT with `sigaction()`, `SA_RESTART`, a global flag | `wc_signal.c` |

## Projects

- **Spec:** `Projs/projN/desc/projectN.md`. Saved Canvas pages are converted to
  Markdown with the `canvas-page-to-markdown` skill. The raw saved HTML embeds
  session tokens and the owner's name and email, so it must never be committed.
- **Grading** (per the proj1 spec; check each new spec): quiz 10%, automated
  tests 40% (public tests plus **hidden** ones), individual oral exam on the
  submitted code 40%, and manual error-checking review 10% (−1 per missed check
  or cleanup).
- **What gets graded:** only files marked `EDIT` in the spec's starter-code
  table. The autograder replaces every other file with the original, so changes
  to `Makefile`, headers or provided `.c` files are silently lost.
- **Error handling the TAs expect:** check every return value that can signal an
  error, report it with `perror("<function>")`, and free or close everything on
  every error path. Output is diffed exactly, so use the exact message strings
  the spec gives. In long-running programs such as the shell, a failing user
  command is reported and the program keeps going. Only errors unrelated to a
  user command end the program.

| Project | What | Due | Notes |
|---|---|---|---|
| proj1 | `swish`, a small job-control shell: tokenizing, `cd`/`pwd`, `fork`/`exec`, `<` `>` `>>`, process groups and `tcsetpgrp`, `fg`/`bg`/`wait-for`/`wait-all` | Fri 10/02 11:59pm (Gradescope) | Reference implementation in `Projs/proj1/claudes-impl/`: its `NOTES.md` covers design choices and oral-exam Q&A, and `make test-extra` runs 25 additional tests |

## Pitfalls

- **Root-owned files.** VS Code's devcontainer runs as root. Committing from
  inside it leaves `.git` objects, the index and refs owned by root, which
  breaks host-side git. It happened on 09-21 and 09-30, and `chown` fixed it.
  Builds made there also leave root-owned `.o` files and binaries in `code/`.
  If you get a permission error, check ownership first.
- **Build output.** Lab03 and the project `code/` dirs have a `.gitignore` for
  binaries, `*.o`, `test_results/`, the submission zip and test scratch files.
  Lab01 and lab02 don't have one, and their submission zips are tracked.
- **Messages the spec doesn't define.** A reference's extra tests expect the
  reference's own wording wherever the spec leaves a message open, such as a
  missing `fg` index. Different wording in the owner's code isn't a bug, so
  report it as a wording difference. To align them, change the reference's
  expected output and its message, not the owner's code. A crash is still a
  real bug.
