# Part 2 (parallel Hadamard product): Claude's implementation

A complete implementation of Part 2 of CSCI 4061 Project 2. The three files the
spec marks `EDIT` are written: `hadamard_pipe.c`, `hadamard_shmem.c` and
`hadamard_threads.c`. The only other change is the `Makefile`, which gains
targets for the extra tests and keeps the starter's build recipe. The autograder
would replace it anyway. `hadamard.h`, `hadamard_utils.c`,
`hadamard_extension.c`, `hadamard_main.py`, `setup.py`, `pyproject.toml` and the
provided tests are the unmodified starter files, as of the 10-22 update (see
"The 10-22 starter update" below).

## Status

| Check | Result |
| --- | --- |
| Provided tests (`make test`) | 18/18 pass |
| Extra tests (`make test-extra`, in `test_cases/extra/`) | 30/30 pass |
| Repeatability | Both suites passed 5 rounds in a row |
| `gcc -Wall -Wextra -Werror` on the three EDIT files, with `-fsyntax-only` and again with `-O2 -c` | Clean |
| Python's own build of the module (`-Wall -Werror` since the 10-22 update) | Clean |
| `clang-format` 19, starter style, `--dry-run --Werror` | Clean |
| Valgrind on the parent and every forked child (extra test 30, plus a one-off run of 18 calls covering every error path) | 0 memory errors, and no leak record of any kind allocated from this code |
| mat_03 (1024x1024), pipe, 4 workers, measured on a 16-core host | 0.08 s with output to `/dev/null`. As a testius test it takes about 1.1 s, almost all of it testius's fixed 0.5 s start delay and moving 7.9 MB through its pty |

How the valgrind check works (`test_cases/extra/resources/valgrind_leaks.sh`,
extra test 30): `PYTHONMALLOC=malloc valgrind --leak-check=full
--show-leak-kinds=all --errors-for-leak-kinds=none` runs pipe, shmem and threads
(3 workers each) on the 2x3 pair. Valgrind follows each `fork()` on its own, so
it checks 7 processes: the parent plus 3 pipe and 3 shmem children. CPython
itself leaves thousands of blocks allocated at exit, so the test counts only
loss records whose allocation stack passes through `hadamard_pipe.c`,
`hadamard_shmem.c` or `hadamard_threads.c`. It counts every kind, not just
"definitely lost". A child calls `exit()` from inside `hadamard_pipe()`, where
its pointers to A, B and the pid array are still on the stack, so a child that
forgets to free them shows up as "still reachable". Each check was run against a
broken copy:

| Broken copy | Test 30 reports |
| --- | --- |
| Pipe children skip `free(A)`, `free(B)` and `free(pids)` | 9 leak records (3 per child). This copy still passes 18/18 provided tests and every other extra test |
| Threads parent skips `free(thread_args)` | 1 leak record ("definitely lost") |
| Thread worker loops to `i <= end` (one past its range) | 1 process with memory errors (invalid read/write past A, B, C) |
| This code | 0, 0 |

The thread_args copy first reported 0. Valgrind writes record numbers above 999
with commas (`loss record 1,421 of 2,083`), which my first `awk` pattern didn't
match. After fixing the pattern it reports 1.

## Running it

From the repo root, in the course image, as root (the image's default; the
build's `pip install -e .` needs it, see below):

```sh
IMG=$(docker images --format '{{.Repository}}:{{.Tag}}' | grep '^vsc-csci_4061' | head -1)
docker run --rm -i --ulimit nofile=1024:1024 --cap-add SYS_PTRACE --security-opt seccomp=unconfined \
  -v "$PWD":/work -w /work/Projs/proj2/claudes-impl/part2 "$IMG" bash -lc 'make && make test-all'
```

Everything the build and tests write here is then root-owned on the host, so run
`make clean clean-tests` inside the container before cleaning up from the host.

```
make                         build hadamard.cpython-310-x86_64-linux-gnu.so
make test                    the 18 provided tests (target unchanged)
make test testnum=4          one provided test, verbose
make test-extra              the 30 extra tests
make test-extra testnum=24   one extra test, verbose
make test-all                both suites (testius empties test_results/ per suite)
python3 hadamard_main.py test_cases/resources/mat_00_a.bin test_cases/resources/mat_00_b.bin threads 3
```

Warning and style checks (inside the container, in this directory):

```sh
gcc -Wall -Wextra -Werror -fsyntax-only $(python3-config --includes) \
    hadamard_pipe.c hadamard_shmem.c hadamard_threads.c
clang-format --style='{BasedOnStyle: LLVM, IndentWidth: 4, ColumnLimit: 100, SpacesBeforeTrailingComments: 4, SpaceAfterCStyleCast: true}' \
    --dry-run --Werror hadamard_pipe.c hadamard_shmem.c hadamard_threads.c
```

**Non-root builds (archive).** Under a non-root uid (`docker run --user`), the
starter's `pip install -e . -qq` fails with EACCES inside the nested `setup.py
develop` (`[Errno 13] Permission denied:
'/usr/local/lib/python3.10/dist-packages/test-easy-install-<pid>.write-test'`, last
seen 10-25). `python3 setup.py -q build_ext --inplace` is the equivalent
workaround: it builds the same `.so` next to the sources, and both suites passed
with it. This Makefile used to switch between the two by uid; it no longer does.

**Warnings fail this build** since the 10-22 starter update: `setup.py` adds
`-Wall -Werror` to Python's own flags (`-Wno-unused-result -Wsign-compare
-DNDEBUG -g -fwrapv -O2 -Wall`, straight from `sysconfig`). `-Wsign-compare` is
in there, so an `int` against `unsigned` comparison that plain `gcc -Wall` lets
through fails here. The `gcc ... -Werror` line above adds `-Wextra`, whose only
extra catch was the unused `self` parameter, hence the `(void) self;` lines.
(The original starter had no `-Werror`, and its `hadamard_utils.c` returned
`PyErr_NoMemory()` from a function returning `int *`, a warning the update
fixed.)

## The 10-22 starter update (optional, per the instructor)

A second `proj2-code.zip` (part2 files dated 10-22) changes only `part2/`; part1
is byte-identical. Diffed against the original in the scratchpad on 10-26:

| File | Change |
|---|---|
| `setup.py` | `extra_compile_args=["-Wall", "-Werror"]`: warnings now fail the build. Also fixes the "hadmard" typo |
| `pyproject.toml` (new) | Declares the setuptools build backend (`setuptools>=64`, `wheel`) |
| `Makefile` | `pip install -e . -qq --no-build-isolation`, so pip uses the image's setuptools 84 instead of fetching one |
| `hadamard_utils.c` | `read_matrix_from_file()` no longer sets a Python error. It `perror()`s or prints to stderr and returns `NULL`; the `return PyErr_NoMemory();` from an `int *` function (the one warning the old build had) becomes `return NULL` |
| `hadamard_pipe.c` | Skeleton now calls `PyErr_SetString(PyExc_OSError, "Failed to read matrix data from file")` after each `NULL` from the reader |
| all three EDIT skeletons | The `worker()` prototype plus empty definition at the bottom becomes one commented-out definition near the top, so the untouched starter compiles under `-Werror` (the old empty `worker()`s were unused-function and missing-return warnings) |

**What it means for an implementation.** Nothing but error handling: the
caller must now raise the Python error after a failed read, or Python reports
`SystemError: <built-in function hadamard_pipe> returned NULL without setting
an exception`. Since the autograder swaps in its own `hadamard_utils.c` and
nobody knows which version, each file here has a `read_failed()` helper that
sets the updated skeleton's `OSError` only `if (!PyErr_Occurred())`. With the
old utils it's a no-op; with the new one it supplies the missing exception.

**Adopted on 10-26.** This directory now carries the updated `Makefile` line,
`setup.py`, `pyproject.toml` and `hadamard_utils.c`. Verified from a clean
build in the container: 18/18 stock, 30/30 extra, strict `gcc -Wall -Wextra
-Werror` and `clang-format` clean. Extra tests 26, 27 and 29 were re-recorded:
a failed read now shows the reader's stderr line (`fopen: No such file or
directory`, or `Unable to read from matrix data file`) followed by
`OSError: Failed to read matrix data from file`, where the old starter raised
`FileNotFoundError: [Errno 2]` or `OSError: Unable to read ...` directly. The
same three EDIT files also pass 18/18 and 30/30 on the original starter
(scratch copy), since `read_failed()` is a no-op when the reader has already
set the error.

## What testius actually checks here

These were checked with a small probe manifest, not just read from the source:

- It runs each command in a **pseudo-terminal**. stdout and stderr both go to
  the compared output: a probe that wrote one line to stderr failed, and that
  line showed up in the "actual" output.
- Because stdout is a terminal, C's `stdout` is line-buffered under testius.
  This matters for the `fflush` story below.
- A per-test `"timeout"` field (whole seconds, default 10) works. A 3 s
  `sleep` with `"timeout": 1` reported "Timed Out".
- The exit status is ignored, except for death by SIGSEGV: a probe that exits
  with status 3 passed.
- Blank lines and runs of whitespace are ignored when comparing. That's why
  extra tests 21–23 pipe the output through `cmp` for a byte-exact check.

## How it works

### Splitting the work (the same helper in all three files)

```c
unsigned chunk_size = n_elems / n_workers + (n_elems % n_workers != 0);  // ceil
*start = min(worker_idx * chunk_size, n_elems);
*end = min(*start + chunk_size, n_elems);
```

The ranges are half-open, `[start, end)`. Workers past the end of the matrix get
`start == end == n_elems`, so their loop runs zero times and needs no special
case. These are the ranges `get_chunk` printed in a scratch harness that
compiled it unchanged:

```
n_elems=12 n_workers=7: [0,2) [2,4) [4,6) [6,8) [8,10) [10,12) [12,12)
n_elems=12 n_workers=4: [0,3) [3,6) [6,9) [9,12)
n_elems=1 n_workers=3: [0,1) [1,1) [1,1)
n_elems=60501 n_workers=20: ... [54468,57494) [57494,60501)
```

`get_chunk` is copied into each file as a `static` function. Only the three EDIT
files count, so there's nowhere shared to put it.

### Threads (`hadamard_threads.c`)

1. Parse `"ssI"` and reject 0 workers (`ValueError`).
2. Read A and B onto the heap (`sh_mem = 0`), then check that the dimensions match.
3. `malloc` C, an array of `pthread_t`, and an array of `struct thread_args`, one per thread.
4. `pthread_create` each thread. On failure: `errno = ret`, then
   `PyErr_SetFromErrno`, and stop creating threads.
5. `pthread_join` every thread that did start, checking each result.
6. If no error was set, `print_matrix`. Then free C, both arrays, A and B on
   every path.

`worker()` is a loop of `c[i] = a[i] * b[i]` over its range, then `return NULL`.

### Shared memory (`hadamard_shmem.c`)

1. Parse `"ssI"` and reject 0 workers.
2. Read A and B with `sh_mem = 1`, which puts them in `MAP_SHARED | MAP_ANONYMOUS`
   segments. Check that the dimensions match.
3. `mmap` C the same way, with `mapped_size = n_elems * sizeof(int)`.
4. `malloc` the pid array, `fflush(stdout)`, then fork one child per worker. Each
   child does `free(pids)` and calls `worker()`, which computes its range and
   `munmap`s all three segments. The child then calls `exit(0)`, or `exit(1)` if
   an `munmap` failed.
5. The parent `waitpid`s every child it forked, retrying on EINTR. A non-zero
   exit status or a death by signal becomes a `RuntimeError`.
6. `print_matrix(C)`, then free the pid array and `munmap` C, A and B.

Error paths:

- B fails to read: unmap A.
- Dimensions don't match: unmap both, each with its own size.
- `mmap` of C fails: unmap A and B.
- `malloc` of pids fails: unmap all three.
- `fork` fails: stop forking, wait for the children already forked, then run the normal cleanup.

### Pipe (`hadamard_pipe.c`)

1. Parse `"ssI"`, reject 0 workers, read A and B onto the heap and check the
   dimensions. The starter already provided the parsing and reading.
2. `malloc` the pid array, call `pipe()`, then `fflush(stdout)`.
3. Fork one child per worker. Each child:
   - closes the read end
   - calls `worker()`, which sends its chunk
   - frees pids, A and B
   - closes the write end
   - calls `exit(status)`
4. The parent closes its write end, `malloc`s C, and `collect_results()` reads
   until EOF.
5. It closes the read end, `waitpid`s every child and checks each status.
6. If no error was set, it prints, then frees C, pids, A and B.

`worker()` packs results into an array of up to 512 `struct element` (exactly
4096 bytes = `PIPE_BUF`). `write_all()` sends the array, retrying on EINTR and
short writes. `collect_results()` reads into a 4096-byte buffer and places every
whole record. If a read ends partway through a record, it `memmove`s those
leftover bytes to the front of the buffer for the next read. It rejects an
out-of-range `idx`, and raises an error if EOF arrives in the middle of a record.

When the parent hits an error after forking (fork, malloc of C, read, a bad
record), it goes straight to closing its read end. Children still writing then
get EPIPE and exit 1, and the parent reaps them. A plain C program would have
them killed by SIGPIPE instead. CPython sets SIGPIPE to "ignore" at startup, and
the children inherit that, so they print `write: Broken pipe` instead. This
happens only if no child still has a read end open, which is one reason each
child closes its own copy. The RLIMIT_NPROC experiment below shows the whole
sequence.

## Design choices the spec doesn't dictate

1. **Half-open ranges.** The starter comments call `end` "the last element".
   But the provided `min()` only works as a clamp on an exclusive bound
   (`min(start + chunk, n)` for the last worker), and an empty range needs
   `start == end`, which an inclusive end can't express. The comments in the
   three EDIT files were updated to match.
2. **Ceiling-sized chunks.** Every worker gets `ceil(n/w)` elements, and the
   last workers may get fewer or none. With this scheme even `w <= n` can leave
   workers idle: 12 elements and 7 workers gives the 7th nothing. The other
   option, `start = (uint64) i * n / w`, gives sizes that differ by at most one.
   The largest chunk, which decides when the slowest worker finishes, is
   `ceil(n/w)` either way. The ceiling is written `n / w + (n % w != 0)` rather
   than `(n + w - 1) / w`, which can wrap. With worker counts now capped at
   `INT_MAX` (item 9), and `n` below 2^30 (the provided reader computes the byte
   size in an `unsigned`), the two forms always agree. The first one just
   doesn't depend on that reasoning.
3. **512 records per `write()`.** Measured on mat_03 with 4 workers, output to
   `/dev/null`, 3 runs each: 512 per write took 0.077–0.078 s, one per write took
   1.09–1.15 s (2.9–3.1 s system time). I had predicted 0.3–1 s for one per
   write, so it came out slower than predicted. With batching, the pipe version
   is as fast as a single thread (0.078 s), because printing the 7.9 MB result
   dominates. Writes of at most `PIPE_BUF` bytes are atomic, so one child's
   records never interleave with another's mid-record.
4. **Partial records are handled but never happened.** An instrumented copy
   counted reads whose length wasn't a multiple of 8. There were none in mat_03
   with 4 and 16 workers, mat_08 with 100, or mat_02 with 7. Short reads did
   happen: 80 of 182 reads on mat_08/100 returned less than asked. POSIX
   doesn't promise record-aligned reads, so the reassembly stays.
5. **`fflush(stdout)` before forking, and children call `exit()`.** See the
   next section.
6. **Exception types.**
   - `ValueError`: a worker count of 0 or less, or mismatched dimensions.
   - `OverflowError` (raised by `PyArg_ParseTuple` itself): a worker count too
     big for an `int`.
   - `OSError` via `PyErr_SetFromErrno`: any failed call that sets `errno`.
     Python picks the subclass from `errno`: `FileNotFoundError` for ENOENT,
     `BlockingIOError` for EAGAIN.
   - `OSError` with the skeleton's message via `read_failed()`: a failed matrix
     read, when the provided reader hasn't already set an error (it does in the
     original starter, not in the 10-22 one; the helper checks).
   - `MemoryError` via `PyErr_NoMemory`: failed `malloc`.
   - `RuntimeError`: a worker that exited non-zero or was killed. No `errno` in
     the parent describes that.
7. **`PyErr_Occurred()` is the error flag.** After the first failure the code
   keeps going through cleanup (close, wait, free, unmap). Each later step only
   raises an error if none is set yet, so the first error is the one Python
   sees.
8. **A real dimension check instead of `assert()`.** Python's flags include
   `-DNDEBUG`, which compiles `assert` out. The spec lets you assume matching
   dimensions, so this goes beyond it (extra test 28).
9. **The worker count is parsed with `"i"`, not the starter's `"I"`.** This
   is a deliberate change to the scaffold the starter gives you in
   `hadamard_pipe.c` (`"ssI"` into an `unsigned`). Whether you follow it is your
   call: `hadamard_main.py` asserts `n_workers > 0`, so the provided tests never
   pass anything else. The reason: `"I"` does no overflow checking. A direct call
   with `-1` arrives as 4294967295 workers, and `2**32` arrives as 0.

   With the old `"I"` parsing, under `docker run --pids-limit 64`, `-1` made
   pipe and shmem fork until `fork()` failed (`BlockingIOError`). The 16 GB pid
   array `malloc` had succeeded first. Threads raised `MemoryError` (its arrays
   need 32 GB). The course container has no pids limit, so pipe and shmem would
   have kept forking until the whole system ran out of processes.

   Now the count goes into an `int`, anything `<= 0` raises `ValueError`, and
   only then does it become an `unsigned` for the chunk arithmetic. `2**32` no
   longer fits an `int`, so `PyArg_ParseTuple` raises `OverflowError` itself.
   Under the same pids limit, the new code rejected `-1` in all three modes
   with `ValueError` and forked nothing. Extra test 25 passes `-1` only to
   threads: against an implementation that still uses `"I"`, the pipe and
   shmem calls would fork-bomb the machine running the tests.

   Without any 0-worker check there is still no divide by zero in this
   structure: `get_chunk` only runs once per worker, and with 0 workers there
   are none. With the check removed (earlier `"I"` version), pipe and threads
   printed uninitialized memory for C (`1497906432` for the 1x1 pair) and
   shmem printed `0`, because a fresh mapping is zero-filled. Nothing crashed,
   but the answer was wrong.
10. **`waitpid()` on the exact pids forked**, not `wait()`, so the call can't
    reap a child the Python program started some other way.

## `exit()` vs `_exit()` in a forked CPython process

The spec says children call `exit()`. `exit()` flushes every C stdio buffer, and
a child's buffers are copies of the parent's at the moment of `fork()`. So if
C's `stdout` buffer holds unprinted output when the parent forks, every child
prints it again.

When does the buffer hold anything? `print_matrix` writes through C's `stdout`.
Python's `print` doesn't: it has its own buffer, and C's `exit()` doesn't flush
it. A run with `print("hello from python")` before a 7-worker pipe call, piped
through `cat`, printed it once even without the `fflush`. The duplication takes
all three of these:

- stdout is a pipe or a file, so C's stdout is fully buffered. Under testius
  it's a pty, so it's line-buffered.
- one process makes two or more calls.
- an earlier call printed less than a buffer's worth, or left a remainder.

`hadamard_main.py` makes one call, so neither the provided tests nor (presumably)
the hidden ones can show it. Results for threads, then pipe with 7 workers, then
shmem with 7, then threads, on the 3x4 pair, piped through `cat` (the right
answer is 12 lines):

| Variant | Lines |
| --- | --- |
| This code (`fflush(stdout)` before forking, `exit()`) | 12 |
| No `fflush`, `exit()` | 75: 7 copies of the first product from the pipe children, then 7 copies of the first two from the shmem children |
| No `fflush`, `_exit()` | 12 |

I predicted 75 before running it. Either fix works. This code keeps `exit()` as
the spec says, and flushes before forking, which also flushes anything a child
might itself print to stdout. Extra test 24 is this exact scenario. Against the
no-`fflush` variant it's the only one of the 30 extra tests that fails.

## Overflow and `-fwrapv`

Every element of all 9 provided pairs is in [-512, 512], so |product| ≤ 262144.
Every element was checked in Python, and no product overflows an `int`. The build
passes `-fwrapv`, which defines signed overflow to wrap. A generated
`[46341, 65536, INT_MIN] ⊙ [46341, 65536, -1]` printed
`-2147479015, 0, -2147483648` in all three modes. Without `-fwrapv`, the same
overflow would be undefined behaviour.

## Experiments with deliberately broken copies (controls)

Each row changes one thing in a scratch copy of this code.

| Change | What happened |
| --- | --- |
| No `fflush(stdout)` before forking | Extra test 24 fails (75 lines instead of 12). The 18 provided and the other 29 extra tests pass |
| Parent never closes its write end | Every test that runs pipe mode hangs, because `read()` never returns 0. Provided: all 9 pipe tests time out (9/18 pass). Extra: 10–12, 16–20, 24, 29 and 30 time out and 21 fails (18/30 pass) |
| Parent never closes its read end | Extra test 29: `leaked file descriptors: 2` (one per successful pipe call) |
| shmem doesn't unmap A when reading B fails | Extra test 29: `leaked shared memory mappings: 2` (missing B, truncated B) |
| Neither the `<= 0` check nor the dimension check (a spec-minimal copy) | 18/18 provided. Only extra tests 25 and 28 fail; `threads` with `-1` gave `MemoryError` there. Test 29 passes, because it uses only failures the spec defines |
| shmem parent doesn't wait for children | Extra test 29: `child processes left: yes`. Provided test 10 (mat_00, 1 worker) printed before the child finished. The other 8 shmem tests passed, so this is a race that output tests catch only sometimes |
| No 0-worker check | Garbage or zeros instead of `ValueError` (item 9 above) |
| shmem child doesn't call `exit()` | Each child falls through into the parent's fork loop. It forks more children of its own, frees `pids` twice, waits on pids that aren't its children, and the survivors return into Python and run the rest of the script. A single `hadamard_main.py` call on the 2x3 pair ended with an abort (signal 6, double free): `RuntimeError: Worker process 52 killed by signal 6`. Across the suites (under `--pids-limit 256`): all 9 provided shmem tests and 12 of 30 extra tests fail. Their output mixes thousands of `free(): double free detected` aborts, `ChildProcessError: [Errno 10] No child processes`, `BlockingIOError` once the pids limit is hit, and parent errors for signal 6 and signal 11. In one run, a child whose pid array held 0 reported `Worker process 0 killed by signal 11`. The process count grows with every fall-through, so run experiments like this one under `docker run --pids-limit` |
| `MAP_PRIVATE` for C | shmem prints all zeros: each child wrote its own copy-on-write copy |
| Pipe children don't free A, B and pids | Still passes all 18 provided tests and 29 of the 30 extra tests. Only test 30 catches it, with 9 leak records |
| threads: `PyErr_SetFromErrno` without `errno = ret` | Still showed `[Errno 11]`: glibc's failed `clone()` happened to leave `errno = EAGAIN`. Correct by accident, and POSIX promises only the return value |

**Fork and thread failure for real.** This ran under an otherwise unused uid with
`ulimit -u 6` and `exec python3`, so only Python and its children counted toward
the limit:

- **pipe, 20 workers, mat_03:** 5 children forked, then the 6th `fork()` raised
  `BlockingIOError: [Errno 11]`. Those five were blocked on a full pipe. Once the
  parent closed its read end they printed `write: Broken pipe`, and all five
  were reaped.
- **shmem, 20 workers:** `BlockingIOError`.
- **threads, 20 workers, tiny matrix:** succeeded. Each thread finished before
  the next was created, so only a few were ever alive at once.
- **threads, 200 workers, mat_03:** `pthread_create` failed with
  `BlockingIOError`, and the threads that did start were joined.
- **Afterwards:** a pipe call with 3 workers still worked, with 0 leaked file
  descriptors and no children left.

`pipe()` failure was forced by lowering `RLIMIT_NOFILE` to 4 after startup:
`OSError: [Errno 24] Too many open files`. The open descriptors were the same
before and after.

## Learning route (for the owner)

The recommended order is the chunk helper, then threads, then shared memory,
then the pipe. Each step adds one new idea to the one before. Check each step
against `make test`, and against the extra tests from a scratch copy, as
AGENTS.md describes. `hadamard_call.py` finds the module three directories up,
so keep it at `test_cases/extra/resources/` in the copy.

**1. The chunk helper.** Write `start`/`end` for worker `i`.
*Checkpoint:* temporarily `printf` the ranges inside your threads loop. Run the
3x4 pair (`test_cases/extra/resources/tiny_3x4_*.bin` here) with 7 workers and
compare against the ranges above. Every index from 0 to 11 should appear exactly
once.
*Classic mistakes:*
- Plain `n / w` with no ceiling. For mat_07 with 20 workers that's 3025 each,
  which covers 60500 of 60501 elements. The provided test "Matrix 7 -- Pipe"
  uses exactly that pair and count.
- An inclusive `end` combined with `min()`, which drops or doubles the last element.

**2. Threads.**
*Checkpoint:* `python3 hadamard_main.py <mat_00 a> <mat_00 b> threads 1 | cmp - test_cases/output/mat_00.txt`.
Then repeat with 3, 16 and 100 threads, then extra tests 1–9.
*Classic mistakes:*
- One `struct thread_args` reused across loop iterations, or `&i` passed to the
  thread. The thread reads it after the loop has changed it.
- Printing before joining.
- Calling `PyErr_SetFromErrno` without `errno = ret`. pthread functions return
  the error number. In this container glibc happens to hide that mistake for
  EAGAIN.
- Forgetting `return NULL` at the end of `worker`. The starter has no return
  there, and the Python build only warns.

**3. Shared memory.**
*Checkpoint:* run the 2x3 pair with 2 workers and compare it with your threads
output. Then switch C to `MAP_PRIVATE` for one run and watch it print zeros.
*Classic mistakes:*
- `MAP_PRIVATE`, or `malloc`, for C. Either way each child writes its own
  copy-on-write copy.
- A child that doesn't `exit()`: it runs on in the parent's code (see the
  double-free row above).
- Not waiting, which passes most tests by luck.
- `munmap` with the wrong size.

**4. The pipe.**
*Checkpoints:*
- Run once *without* the parent's `close(write end)` under `timeout 5`, and see
  the hang.
- Run mat_03 with 4 workers and `cmp` the output.
- Run extra test 24.

*Classic mistakes:*
- The parent not closing its write end: it hangs.
- A child not closing its read end.
- Children not freeing their inherited copies of A and B.
- Sending bare `int`s. The parent can't tell which index a value belongs to,
  because the children's writes arrive in any order.
- Single writes bigger than `PIPE_BUF`, which can interleave with another
  child's.
- Assuming one `read()` returns whole records.
- Following the starter's stale TODO in `worker()`, which says to open the two
  matrix files again. The data is already in memory and arrives through `a`
  and `b`.

**Wording.** Extra tests 25 and 28 expect this code's own messages: `Number of
workers must be at least 1` and `Matrix dimensions do not match`. The spec
defines neither message, and both tests check things the spec lets you assume
away. Different wording in your code, or no such check at all, is a wording or
scope difference, not a bug. Tests 26, 27, 29 and 30 use only behaviour the spec
defines. Don't run extra test 25 against code that still parses with `"I"` and
has no `<= 0` check, outside a container with `--pids-limit`: its `threads` call
with `-1` then asks for 4294967295 threads. On this host that fails fast with
`MemoryError`, but it is not guaranteed elsewhere.

## Oral-exam material: things worth being able to explain

- **Copy-on-write, so why do children still free A and B?** After `fork()` the
  child has its own virtual copy of the parent's heap. Pages are shared
  physically until someone writes, but logically they're the child's
  allocations. Freeing them in the child doesn't touch the parent's copies.
  Not freeing them is a leak in the child: valgrind showed them still reachable
  in each child.
- **Why `struct element`?** Children run concurrently and their writes land in
  the pipe in any order. Sending `{idx, value}` makes each record
  self-describing, so the parent can put each value in its place no matter
  which child sent it or when.
- **PIPE_BUF atomicity.** POSIX guarantees that a `write()` of at most
  `PIPE_BUF` bytes (4096 on Linux) to a pipe is not interleaved with other
  writers' data. Each batch here is at most 512 × 8 = 4096 bytes, so records
  from different children never mix mid-record. Larger writes can be split and
  interleaved.
- **Why does `read()` return 0 only after every write end is closed?** EOF on
  a pipe means no process could ever write more. Every child, and the parent
  itself, holds a copy of the write end. If the parent doesn't close its own
  copy, it waits forever for data only it could send (the "Timed Out" control).
- **Why no locking?** Every worker reads and writes a disjoint index range of C,
  and nobody reads C until all workers have finished: after `waitpid` or
  `pthread_join`, which also make the workers' writes visible. In the pipe
  version only the parent writes C.
- **How do children get the segments?** A `MAP_SHARED | MAP_ANONYMOUS` mapping
  made before `fork()` is inherited at the same address and refers to the same
  physical pages. So the child can use the parent's pointer as-is, and its
  writes are visible to the parent. `MAP_PRIVATE` would give the child a
  copy-on-write copy instead (the all-zeros control).
- **What is `mapped_size` for?** `munmap(addr, length)` needs the length of the
  mapping. The worker unmaps its view of A, B and C before exiting. That only
  removes the child's mapping: the parent's mapping and the shared pages stay.
- **Threads vs processes.** Threads share one address space, so plain `malloc`
  works for C and there's nothing to send back. Processes need shared memory or
  a pipe, because after `fork()` ordinary memory is private to each process.
- **`PyArg_ParseTuple(args, "ssi", ...)`.** `s` gives a `const char *` into the
  Python string, owned by Python (don't free it). `i` gives an `int` and raises
  `OverflowError` if the value doesn't fit. The starter's `I` gives an
  `unsigned int` *with no overflow check*: `2**32` arrives as 0, and `-1` as
  4294967295 (design choice 9). If
  parsing fails, it has already set an exception (a `TypeError` for a wrong argument type),
  so just return NULL.
- **`Py_RETURN_NONE`.** Returns Python's `None` after increasing its reference
  count. Returning a `PyObject *` means returning a new reference, and `None`
  is a counted object like any other (in 3.10).
- **When `PyErr_SetFromErrno` applies.** Only right after a call that sets
  `errno` on failure: `pipe`, `fork`, `read`, `waitpid`, `mmap`, `munmap`,
  `fflush`. Not for `pthread_*`, which return the error (set `errno = ret`
  first). Not for a failed child, which has no `errno` in the parent. Not for
  `malloc` (use `PyErr_NoMemory`).
- **The GIL.** The calling thread holds the GIL for the whole call. Worker
  threads and child processes never call the Python C API, so they never need
  it. Children `exit()` without ever returning into the interpreter.
  Exceptions are raised only in the parent, on the thread that holds the GIL.
- **More workers than elements.** The extra workers get an empty range, and a
  process or thread is still created for each. They do nothing and exit 0 (in
  pipe mode, after writing nothing). Tests 16–20 run 50 workers on matrices of
  1–12 elements.
- **Signed overflow.** The products of the provided matrices fit easily. The
  build's `-fwrapv` makes overflow wrap rather than be undefined (see above).

## Extra tests

`test_cases/extra/tests_extra.json`. Each test's intent is in its manifest
`description`. Helpers live in `test_cases/extra/resources/`:

- `gen_matrices.py` writes the small `.bin` pairs and every expected output
  that isn't a provided `mat_0N.txt`. Re-running it reproduces them byte for byte.
- `hadamard_call.py` calls the module directly. That skips `hadamard_main.py`'s
  `assert n_workers > 0`, makes several calls in one process, prints each
  exception as `Type: message`, and with `--check-leaks` reports leaked file
  descriptors, leaked shared mappings and leftover children.
- `valgrind_leaks.sh` (test 30) runs `hadamard_call.py` under valgrind and
  prints three summary lines that don't depend on the run. The logs stay in
  `test_results/valgrind/`.

| # | What it checks |
| --- | --- |
| 1–9 | Threads on all 9 provided pairs with 1, 2, 3, 16, 5, 7, 33, 100 and 64 workers |
| 10–12 | Pipe with 7, 100 and 64 workers (64 on 32768 elements = one full `PIPE_BUF` write per child) |
| 13–15 | Shared memory with 13, 100 and 64 workers |
| 16–20 | 1x1, 1x5, 5x1, 3x4 and 2x3 pairs. Every mode with 7, then 50 workers, in one process |
| 21 | mat_03 pipe, 4 workers, byte-exact via `cmp`, killed after 3 s |
| 22–23 | mat_03 shmem (9) and threads (13), byte-exact |
| 24 | No duplicated output when stdout is a pipe and one process makes several calls |
| 25 | 0 workers (every mode) and -1 (threads) raise `ValueError`; `2**32` raises `OverflowError` in every mode |
| 26 | A missing first file, then a missing second file, raise `OSError` in every mode (the reader's `fopen:` line on stderr first) |
| 27 | A truncated second file raises `OSError` in every mode |
| 28 | Mismatched dimensions (2x3 vs 3x2) raise `ValueError` in every mode |
| 29 | 13 calls mixing successes with the failures the spec defines (missing and truncated files) leak no descriptors, no mappings and no children |
| 30 | Under valgrind (parent and all forked children): no memory errors, and no heap block allocated from this code left at exit. About 2.5 s on a 16-core host; the manifest allows 30 s |

Tests 25–29 end with (or include) a successful call, to show the module still
works after an error.

## Not verified

- The failure paths of `malloc` for C, `mmap` for C, `read()`, `munmap` and
  `waitpid` were never forced. They are covered only by reading the code. Fork,
  `pthread_create`, `pipe()`, file open and short-read failures were forced,
  as described above.
- Hidden tests aren't known. If they go through `hadamard_main.py` like the
  provided ones, they never call with 0 workers or make more than one call per
  process.
- All timings were measured on one 16-core host, a few runs each. They are not
  a benchmark, and other machines will differ.
- `-1` in pipe and shmem mode is not in the test suite (see design choice 9).
  It was run once against the new code under `--pids-limit 64`, where it
  raised `ValueError`.
