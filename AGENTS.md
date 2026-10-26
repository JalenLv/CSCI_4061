# CSCI 4061 (Fall 2026) — agent guide

Coursework for UMN CSCI 4061, Introduction to Operating Systems: C systems
programming on Linux (processes, I/O, signals). Everything here is graded work
the owner writes themselves. That shapes most of the rules below.

## Ground rules

- **The owner does their own coursework.** Explain, review, run tests and
  report results. Don't edit their solutions (lab `code/*.c`, `QUESTIONS.txt`,
  `Projs/*/code/`) unless asked; describe bugs and let them fix them. Reading
  their files is fine.
- **Agent-owned directories** (`Projs/projN/claudes-impl/`, see Layout) are
  the exception: agents may edit them freely.
- Don't commit or push unless asked. Check `git log` first for message
  conventions and existing commits. No `Co-Authored-By` on the owner's work
  (any `code/` change, their quiz answers); commit it apart from agent work.
- `HWs/` is out of scope for now.

## Layout

```
.devcontainer/             Course Docker image
Labs/labNN/labNN.pdf       Lab slides
Labs/labNN/code/           Unpacked starter zip, completed in place
Projs/projN/desc/          Spec, converted from the Canvas page to projectN.md, plus figures
Projs/projN/quiz/          Project quiz, converted from Canvas to QUESTIONS.md, plus the agent's answered copy
Projs/projN/code/          Unpacked starter zip, completed in place (the owner's submission)
Projs/projN/claudes-impl/  Agent-owned reference implementation, if one was requested
```

The history follows a fixed pattern: `Init labNN` or `Init projN` commits the
starter code and spec, and `Complete labNN` commits the finished work.

## The devcontainer

Everything builds and runs in the course image, defined by `.devcontainer/`:
Ubuntu 22.04 with gcc, gdb, valgrind, clang-format 19, Python 3.10, testius and
socrates, plus the POSIX man pages. The owner uses it through VS Code; agents
run it with `docker run`. It's likely already built locally, and the tests
are only guaranteed to behave the same inside it. From the repo root:

```sh
IMG=$(docker images --format '{{.Repository}}:{{.Tag}}' | grep '^vsc-csci_4061' | head -1)
docker run --rm -i \
  --ulimit nofile=1024:1024 --cap-add SYS_PTRACE \
  --security-opt seccomp=unconfined \
  -v "$PWD":/work -w /work/<path/to/code> \
  "$IMG" bash -lc '<command>'
```

- Use `-it` instead of `-i` to drive a program interactively or to get a shell.
- The container runs as root (the image's default, and VS Code's). Don't pass
  `--user`: some starter build steps only work as root (a reference's
  `NOTES.md` records the cases).

## Building and testing

Build flags are always `-Wall -Werror -g`, so any warning fails the build. Each
`code/` has a `Makefile` with `make`, `make test`, `make clean` and
`make clean-tests`; labs add more (see Labs), projects vary (read the Makefile).

**testius** is the course test runner. Each `code/` dir has a JSON manifest
under `test_cases/`.

- `make test testnum=N` runs one test. All project Makefiles support it, some
  labs do; check the Makefile. The runner itself takes `-v -n N`.
- Input files can contain `^C`, `^Z` and `^D` lines; testius sends them as
  real keystrokes through a pty.
- `{{…}}` in an input or expected-output file holds a shell command, and
  testius replaces it with the command's stripped stdout before the test runs
  (`{{pwd}}`, `{{echo $HOME}}`, `{{wc test_cases/resources/nums.txt}}`). One
  per line; any stderr aborts the test.
- `test_results/` is emptied at the start of each run. `*-results.tmp` is a
  side-by-side diff; `*-valgrd.tmp` the valgrind log when `use_valgrind` is set
  (any non-empty string, even `"n"`; `false` turns it off).

**Extra tests from a reference implementation** live in its `test_cases/extra/`.
They're written to the spec, so they stand in for hidden tests against the
owner's code. Don't add them to `code/`: make a scratch copy of it, drop the
extra tests into the copy's `test_cases/`, and run testius there. The
reference's `NOTES.md` has the commands.

## Code style

There's no `.clang-format` in the repo. The starter code matches `{BasedOnStyle:
LLVM, IndentWidth: 4, ColumnLimit: 100, SpacesBeforeTrailingComments: 4,
SpaceAfterCStyleCast: true}`.

## Labs

Each lab has a QUIZ and a CODE part; `make zip` builds the Gradescope upload.

- **QUIZ:** change `( )` to `(X)` in `QUESTIONS.txt` and nothing else.
  `socrates` checks against a hash, so the answers can't be read from the repo.
  `QUESTIONS.txt.bak` is the pristine copy.
- **CODE:** complete the `.c` files the lab names.
- **Targets:** `make test-quiz`, `make test-code`, `make test`, `make zip`. The
  targets differ slightly between labs, so check `make help`.

| Lab | Topic |
|---|---|
| lab01 | `fork()`, `exec()`, `wait()` |
| lab02 | Signals, job control, file descriptors, `dup2()` redirection |
| lab03 | Catching SIGINT with `sigaction()`, `SA_RESTART`, a global flag |
| lab04 | Pipes: `pipe()`, `fork()`, `dup2()` to run `sort -n … \| tail -n 10` |
| lab05 | Shared memory with `mmap()`, splitting work across forked worker processes |
| lab06 | A Python C extension: `PyArg_ParseTuple`, building a list to return, `PyErr_*` |

## Projects

- **Spec:** `Projs/projN/desc/projectN.md`, made from the saved Canvas page
  with the `canvas-page-to-markdown` skill. Never commit the raw HTML: it
  embeds session tokens and the owner's name and email.
- **Quiz:** `Projs/projN/quiz/QUESTIONS.md`, same skill: `### Question N`, one
  `- [ ]` per choice (the owner marks `[X]`), header like the spec's.
- **Grading** (both specs so far; check each new one): quiz 10%, automated
  tests 40% (public plus hidden), oral exam on the submitted code 40%, manual
  error-checking review 10%.
- **What gets graded:** only the files marked `EDIT` in the spec's
  starter-code table; the autograder restores every other file.
- **Error handling the TAs expect:** check every return value that can signal an
  error, report it the way the spec says (usually `perror("<function>")`), and
  free or close everything on every error path. Output is diffed exactly, so
  use the exact message strings the spec gives. What happens after an error
  differs by program: read the spec's error-handling section.

| Project | What | Due | Reference notes |
|---|---|---|---|
| proj1 | `swish`, a job-control shell: `cd`/`pwd`, `fork`/`exec`, `<` `>` `>>`, process groups, `fg`/`bg`/`wait-for`/`wait-all` | Fri 10/02 11:59pm (Gradescope) | `Projs/proj1/claudes-impl/NOTES.md` |
| proj2 | `part1/`: `\|` pipelines for `swish` (`pipe()`, `dup2()`, a provided `run_command()`). `part2/`: parallel Hadamard product three ways (pipe, shared memory, threads) as a Python C extension | Fri 10/30 11:59pm (Gradescope); quiz Fri 10/16 | `Projs/proj2/claudes-impl/NOTES.md` |

## Pitfalls

- **Root-owned files.** Docker may be rootful, so container root is host root
  and everything a build or test writes into the mount is root-owned on the
  host (not so under rootless Docker or podman). The host user then can't
  delete or overwrite those files. So run `make clean clean-tests` inside the
  container before host-side `git clean`, edits or deletes, and never commit
  from inside it: that leaves `.git` root-owned and breaks host git (`chown`
  fixes it). On a permission error, check `find . -user root` first.
- **Build output.** Lab01 and lab02's `code/` have no `.gitignore`; every
  later one does (binaries, `*.o`, `test_results/`, the submission zip, test
  scratch files). The lab01, lab02 and lab04 zips are tracked.
- **Provided objects.** Starter code can ship prebuilt objects the Makefile
  links against (proj2's `part1/swish_funcs_provided_*.o`). A blanket `*.o`
  rule hides them, so whitelist them (`!swish_funcs_provided_*.o`) and keep
  them tracked.
- **Messages the spec doesn't define.** Where the spec leaves a message open,
  a reference's extra tests expect the reference's wording. Different wording
  in the owner's code is a wording difference, not a bug; to align them, change
  the reference, not the owner's code. A crash is still a bug.
