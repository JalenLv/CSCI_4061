# Project 3: Claude's reference implementation

A complete implementation of CSCI 4061 Project 3, written alongside (never
copied into) the owner's own work in `Projs/proj3/code/`. Only the files the
spec marks `EDIT` carry solution code:

| Part | Graded files | Notes |
| --- | --- | --- |
| 1 (C) | `part1/http.c`, `http_server.c`, `connection_queue.h`, `connection_queue.c` | [part1/NOTES.md](part1/NOTES.md) |
| 2 (Python) | `part2/http_handler.py`, `http_server.py`, `connection_queue.py` | [part2/NOTES.md](part2/NOTES.md) |

Everything else is the unmodified starter, with two agent-owned exceptions:
the three `Makefile`s gain extra-test targets, and each part has a
`test_cases/extra/` suite of its own.

## Status

| Suite | Result |
| --- | --- |
| Part 1 provided tests | 2/2 |
| Part 1 extra tests | 18/18 |
| Part 1 valgrind (extra 17) and ThreadSanitizer (extra 6, 18) | 0 errors, all heap blocks freed; 0 warnings |
| Part 2 provided tests | 2/2 |
| Part 2 extra tests | 15/15 |
| `clang-format` (the starter's style) | clean |

All four suites passed together in one container run (`make test-all`, about
75 s), from a clean build, as root, on 11-04.

The extra tests were also run against 26 copies, each with a classic mistake
(both parts' NOTES list them and what catches each), twice each in fresh
containers; the unmodified control passed every run. 22 of the 26 can pass both
provided tests. The extra suites catch all but one: ignoring a failed write
while the signal mask is right, which changes nothing a client can see.

## Running it

From the repo root, in the course container:

```sh
IMG=$(docker images --format '{{.Repository}}:{{.Tag}}' | grep '^vsc-csci_4061' | head -1)
docker run --rm -i --ulimit nofile=1024:1024 --cap-add SYS_PTRACE --security-opt seccomp=unconfined \
  -v "$PWD":/work -w /work/Projs/proj3/claudes-impl "$IMG" bash -lc 'make && make test-all'
```

Inside a shell (`-it`, ending with plain `bash`):

```sh
make                      # both parts
make test-part1           # 2 provided Part 1 tests, ports 8000-8001
make test-extra-part1     # 18 extra Part 1 tests, ports 8011-8028
make test-part2           # 2 provided Part 2 tests, ports 8003-8004
make test-extra-part2     # 15 extra Part 2 tests, ports 8051-8065
make test-all             # all four
cd part1 && make test-extra testnum=12    # one extra test, verbose
```

Pass `port_base=N` to any of them to shift every port by `N - 8000`.

- **Each suite gets its own ports.** A port can't be bound again for about a
  minute after the server has closed connections on it, so back-to-back runs
  on the same ports fail with `Address already in use`. The starter's
  top-level `make test` puts Part 2 on ports 3 and 4 by accident (an undefined
  variable), which works because the container runs as root.
- **Run `make clean clean-tests` inside the container before host-side
  work.** Build and test output is root-owned on the host. On 11-04 that left
  nothing root-owned behind.

## Running the extra tests against the owner's code

The extra suites are written to the spec, so they stand in for the hidden
tests. The owner's `code/` gets no new files, so run them from a scratch copy:

```sh
S=/tmp/proj3-check && rm -rf "$S" && mkdir -p "$S"
cp -r Projs/proj3/code/part1 "$S/part1"
cp -r Projs/proj3/claudes-impl/part1/test_cases/extra "$S/part1/test_cases/"
cp -r Projs/proj3/code/part2 "$S/part2"
cp -r Projs/proj3/claudes-impl/part2/test_cases/extra "$S/part2/test_cases/"
docker run --rm -it --ulimit nofile=1024:1024 --cap-add SYS_PTRACE --security-opt seccomp=unconfined \
  -v "$S":/work -w /work "$IMG" bash
# inside:
cd part1 && make && PORT_BASE=8010 testius test_cases/extra/tests_extra.json
cd ../part2 && make && PORT_BASE=8050 testius test_cases/extra/tests_extra.json
# before leaving, so the host's rm -rf "$S" works next time:
cd .. && make -C part1 clean clean-tests && make -C part2 clean clean-tests
```

Add `-v -n 12` to run one test verbosely. `PORT_BASE` must be set: test `n`
binds port `PORT_BASE + n`.

**What the spec leaves open.** A few extra tests depend on choices the spec
doesn't make. Where the owner's code differs, it's a difference, not a bug:

| Test | Expects | Spec says |
| --- | --- | --- |
| Part 1 extra 5, 11, 12; Part 2 extra 5, 10, 11 | Dequeue keeps returning queued connections after shutdown until the queue is empty | The spec says exactly this for Part 1 ("threads can continue to perform a dequeue operation until the queue becomes empty"); `connection_queue.h`'s comment says the opposite. Part 2 says "the same way as your C code" |
| Part 1 extra 2, Part 2 extra 2 | 404 for `/quote.txt/more` (`stat` fails with `ENOTDIR`) | Only that a missing resource gets 404 |
| Part 2 extra 1 | `Content-Type: text/jpeg` for `.jpg`, the starter's table | Nothing; the standard type is `image/jpeg`, which Part 1's table uses |
| Part 1 and 2 extra 16 | The failing second server's output contains "Address already in use" | Suggests `perror()`, whose wording that is; Python's exception text has it too |

Extra test 13 (malformed requests) accepts any behavior that closes the
connection, and no test checks a server's error messages.

## Learning route

Each part's `NOTES.md` has a checkpointed route, a table of classic mistakes
(each reproduced in a deliberately broken copy and matched to the tests that
catch it) and oral-exam Q&A. The overall order:

1. **Read the spec end to end**, then Part 1's "How it works".
2. **Part 1, sequential first.** Socket setup and one client at a time, all in
   `main`. Provided test 1 and extra tests 1–4 need nothing more. Test 4 is the
   request-reading loop most first attempts get wrong: the end of the request
   can arrive split across two reads.
3. **The queue in isolation** (extra 5 and 6), then the pool (provided test 2,
   extra 7–9), then shutdown (extra 10–12). Shutdown is where the hangs live:
   broadcast, not signal; `while`, not `if`; no `SA_RESTART`.
4. **Robustness, valgrind and TSAN** (extra 13–18). The spec warns TSAN may be
   a hidden test.
5. **Part 2** is the same design in a third of the code. The one new idea is
   the accept timeout, and Part 2's NOTES measures why it's needed.
6. **Error-handling pass on every file.** Every call that can fail is checked
   and reported; on a client's failure the server moves on, on a setup failure
   it cleans up and exits 1. Part 1's NOTES has fault-injection results for
   the paths no test reaches.
7. **Oral exam.** Run the experiments rather than reading the answers: remove
   the broadcast and watch the join hang, add `SA_RESTART` and watch SIGINT do
   nothing.

## Layout

```
claudes-impl/
  Makefile      starter + test-part{1,2}, test-extra-part{1,2}, test-extra, test-all (separate ports)
  NOTES.md      this file
  part1/        starter + the four EDIT files, NOTES.md, test_cases/extra/ (18 tests)
  part2/        starter + the three EDIT files, NOTES.md, test_cases/extra/ (15 tests)
```

Both `test_cases/extra/resources/` hold one script per test, sharing `lib.sh`
(start, stop and wait for the server) and `http_client.py` (a raw-socket
client that prints what it saw). Part 1's also has `queue_test.c`, Part 2's
`queue_test.py`.
