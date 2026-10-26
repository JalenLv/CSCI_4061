# Project 2: Claude's reference implementation

A complete implementation of CSCI 4061 Project 2, written alongside (never
copied into) the owner's own work in `Projs/proj2/code/`. Only the files the
spec marks `EDIT` carry solution code:

| Part | Graded file(s) | Notes |
| --- | --- | --- |
| 1 | `part1/swish_funcs.c` | [part1/NOTES.md](part1/NOTES.md) |
| 2 | `part2/hadamard_pipe.c`, `hadamard_shmem.c`, `hadamard_threads.c` | [part2/NOTES.md](part2/NOTES.md) |

Everything else is the unmodified starter, with two agent-owned exceptions:
the three `Makefile`s gain `test-extra` / `test-all` targets, and each part has
a `test_cases/extra/` suite of its own.

## Status

| Suite | Result |
| --- | --- |
| Part 1 provided tests (`make test-part1`) | 8/8 |
| Part 1 extra tests (`make test-extra-part1`) | 20/20 |
| Part 1 valgrind (shell plus every forked child, incl. failed-exec children; extra test 19 checks the children's reports itself) | 0 errors, 0 bytes in use at exit |
| Part 2 provided tests (`make test-part2`) | 18/18 |
| Part 2 extra tests (`make test-extra-part2`), incl. all nine matrix pairs with threads | 30/30 |
| Part 2 valgrind (parent, every forked child, every thread; extra test 30 runs it itself) | 0 errors, no leak record from the three EDIT files |
| Part 2 `gcc -Wall -Wextra -Werror` on the three EDIT files | clean (the build itself has `-Werror` since the 10-22 starter update) |
| `clang-format` (LLVM, 4-space, 100 columns, as the starter) | clean |

Each number above was reproduced from a clean build in the course container,
independently of the run that produced it, and again as root on 10-25.

## Before you start: your `code/part1` was missing two starter files

The owner's `Projs/proj2/code/part1/` had no `swish_funcs_provided_x86_64.o`
(nor the `aarch64` one). The starter zip ships both and `part1/Makefile` links
against `swish_funcs_provided_$(uname -m).o`, so `make` there fails at the link
step without them. The owner has since copied them in: on 10-25 both are in
`code/part1/` (untracked by git) and byte-identical to the pristine copies this
directory keeps from the official zip. If they go missing again:

```sh
cp Projs/proj2/claudes-impl/part1/swish_funcs_provided_*.o Projs/proj2/code/part1/
```

They are not solution code, and the autograder brings its own copy anyway.

## Running it

From the repo root, in the course container (the image is built locally),
which runs as root:

```sh
IMG=$(docker images --format '{{.Repository}}:{{.Tag}}' | grep '^vsc-csci_4061' | head -1)
docker run --rm -i --ulimit nofile=1024:1024 --cap-add SYS_PTRACE --security-opt seccomp=unconfined \
  -v "$PWD":/work -w /work/Projs/proj2/claudes-impl "$IMG" bash -lc 'make && make test-all'
```

That builds both parts and runs all four suites. For a shell inside, use `-it`
and end the command with plain `bash`. Then, inside:

```sh
make                  # both parts
make test-part1       # the 8 provided Part 1 tests (valgrind on)
make test-extra-part1 # the 20 extra Part 1 tests
make test-part2       # the 18 provided Part 2 tests (pipe and shmem)
make test-extra-part2 # the 30 extra Part 2 tests (threads, odd counts, tiny matrices, errors, leaks)
make test-all         # all four suites, one after the other

cd part1 && make test-extra testnum=4    # one extra test, verbose
cd part2 && make test-extra testnum=24   # likewise
```

Two environment facts worth knowing:

- **The container runs as root, so everything it writes is root-owned.** Root
  is the image's default and what the VS Code devcontainer uses, and the part2
  build's `pip install -e .` needs it (part2/NOTES.md records the non-root
  failure). Build and test output in the mount (`swish`, `*.o`, `out.txt`,
  `build/`, `hadamard.egg-info/`, the `.so`, both `test_results/`) is therefore
  root-owned on the host, and a host-side `rm -rf` of it fails with
  `Permission denied`. Run `make clean clean-tests` from inside the container
  before host-side work (`git clean`, editors, deleting files). On 10-25 that
  left nothing root-owned behind.
- **testius empties `test_results/`** at the start of every run, so after
  `test-all` only the last suite's `*-results.tmp` and `*-valgrd.tmp` files
  survive. Run one suite when you want to read its logs.

## Running the extra tests against the owner's code

The extra suites are written to the spec, so they stand in for the hidden tests.
The owner's `code/` gets no new files, so run them from a scratch copy
(AGENTS.md describes the same procedure):

```sh
S=/tmp/proj2-check && rm -rf "$S" && mkdir -p "$S"
cp -r Projs/proj2/code/part1 "$S/part1"
cp -r Projs/proj2/claudes-impl/part1/test_cases/extra "$S/part1/test_cases/"
cp -r Projs/proj2/code/part2 "$S/part2"
cp -r Projs/proj2/claudes-impl/part2/test_cases/extra "$S/part2/test_cases/"
docker run --rm -it --ulimit nofile=1024:1024 --cap-add SYS_PTRACE --security-opt seccomp=unconfined \
  -v "$S":/work -w /work "$IMG" bash
# inside:
cd part1 && make && testius test_cases/extra/tests_extra.json
cd ../part2 && make && testius test_cases/extra/tests_extra.json
# before leaving, so the host's rm -rf "$S" works next time:
cd .. && make -C part1 clean clean-tests && make -C part2 clean clean-tests
```

`part2/test_cases/extra/resources/hadamard_call.py` locates the module three
directories up, so keep the copied layout as it is.

**Wording the spec leaves open.** A few extra tests expect this
implementation's own messages. Different wording in the owner's code is a
wording difference, not a bug (a crash still is):

| Test | Expects | Spec says |
| --- | --- | --- |
| Part 1, extra 13 (empty command such as `a \| \| b`) | `Error: Empty command in pipeline` on stderr | nothing; the provided `run_command()` segfaults on an empty command |
| Part 2, extra 25 (`n_workers` of 0 and −1; `2**32` expects Python's own `OverflowError`) | `ValueError: Number of workers must be at least 1` | nothing; `hadamard_main.py` asserts `n_workers > 0` before calling |
| Part 2, extra 28 (mismatched dimensions) | `ValueError: Matrix dimensions do not match` | may be assumed away; the starter's `assert` is compiled out by `-DNDEBUG` |
| Part 2, extra 28's mismatch check and 25's `−1` need code the starter scaffold doesn't have | see the next paragraph | |

This implementation parses the worker count with the `"ssi"` format (a signed
`int`, rejected when `<= 0`) instead of the starter's `"ssI"` in
`hadamard_pipe.c`. With `"I"`, a direct call with `-1` becomes 4294967295
workers and forks until `fork()` fails. Nothing graded can reach that (the
test driver asserts `n_workers > 0`), so keeping the starter's `"I"` is a
legitimate choice; [part2/NOTES.md](part2/NOTES.md) explains both.

## Learning route

Each part's `NOTES.md` has a step-by-step route with a checkpoint per step and a
table of classic mistakes, each one reproduced in a deliberately broken copy
and matched to the tests that catch it. The overall order:

1. **Read the spec once end to end**, then the "How it works" section of the
   part you're about to build. Keep the per-part NOTES open for its checkpoints,
   not for its code.
2. **Part 1 first.** It's one function pair, it reuses Project 1 habits
   (`fork`, `dup2`, `waitpid`), and the hidden tests are about shapes you can
   test yourself: more than three commands, `<` first, `>`/`>>` last. Follow
   [part1/NOTES.md › Learning route](part1/NOTES.md#learning-route): parse
   without forking, then two commands and one pipe, then the general case, then
   failure paths, then descriptor hygiene. The three bugs that pass all 8
   provided tests but fail the extra ones (parent closes only write ends, child
   keeps its dup'd fds, launch-and-wait one command at a time) are the ones to
   understand before the oral exam.
3. **Part 2, threads first.** It's the shortest file and has no provided tests,
   so write it and check it against the extra suite (tests 1–9). Then shared
   memory, which is the same loop with `mmap` and `fork`, then the pipe, which
   adds record framing and EOF rules. Follow
   [part2/NOTES.md › Learning route](part2/NOTES.md#learning-route-for-the-owner).
   Two traps in the starter itself: the stale TODO inside `hadamard_pipe.c`'s
   `worker()` tells you to reopen the matrix files (don't; the data arrives via
   `a` and `b`), and the `assert` on matrix dimensions is compiled out.
4. **Error-handling pass last, on every file.** The TA rubric is "every call
   that can fail is checked, reported, and everything is released on that
   path". The per-part NOTES list what each path owns at each point. Verify with
   valgrind in Part 1 (every `*-valgrd.tmp`, including the ones for children
   that fail to exec) and with the fault-injection recipes in Part 2.
5. **Oral exam.** Both NOTES end with a Q&A section. Work through it by running
   the experiments, not by reading the answers: for instance remove the
   parent's `close()` loop and watch the hang, or send bare `int`s over the
   pipe and watch the result matrix scramble.

Two spec facts shape most of the design, and come up in questions:

- A pipeline of `n` commands needs `n − 1` pipes, all created in the parent
  before any fork, and **every** process must close every end it doesn't use
  before anyone waits. EOF on a pipe only arrives when the last write end in the
  whole system is closed.
- Work splits into half-open index ranges `[start, end)` over the flattened
  matrix, with a ceiling-sized chunk. Workers past the end of the data get an
  empty range and simply finish; that's how "more workers than elements" works.

## Layout

```
claudes-impl/
  Makefile            top-level starter Makefile + test-extra-part{1,2}, test-extra, test-all
  NOTES.md            this file
  part1/              starter + swish_funcs.c, NOTES.md, test_cases/extra/ (20 tests; resources/ has
                      words.txt and the child_leaks.sh valgrind driver for test 19)
                      swish_funcs_provided_{x86_64,aarch64}.o from the official zip
  part2/              starter + the three hadamard_*.c, NOTES.md, test_cases/extra/ (30 tests)
                      test_cases/extra/resources/gen_matrices.py regenerates the tiny matrices
                      and every extra expected output; hadamard_call.py drives direct calls;
                      valgrind_leaks.sh is the driver for leak test 30
```
