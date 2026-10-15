# CSCI 4061 -- Project 2 Quiz (answered)

> Archived from the Canvas quiz page
> <https://canvas.umn.edu/courses/579183/quizzes/1205758>
>
> | | |
> |---|---|
> | **Due** | Fri Oct 16, 2026 11:59pm |
> | **Points** | 18 (18 questions, 1 pt each) |
> | **Available** | Oct 12, 2026 12:00am - Oct 16, 2026 11:59pm |
> | **Allowed attempts** | 10 |
> | **Time limit** | none |

> **Note:** Answered copy of `QUESTIONS.md`; every choice cites the spec (`desc/project2.md`) or the starter code. Q5 carries a caveat.

---

## Quiz Instructions

This quiz will cover some of the important elements of the Project 2 assignment specification and starter code. Start by reading the full [Project 2 Assignment](https://canvas.umn.edu/courses/579183/assignments/5444400) spec carefully. You should also look over the project starter code, particularly any header (`.h`) files.

## Questions

### Question 1

When in the terminal and working on Project 2, in which current working directories will the `make test` command work?

- [ ] You can run `make test` in the `part1` subdirectory to run tests for Part 1 of the project.
- [ ] You can run `make test` in the `part2` subdirectory to run tests for Part 2 of the project.
- [ ] You can run `make test` in the top-level `proj2-code` directory to run tests for both parts of the project.
- [X] All of the above

> **Why:** All three Makefiles have a `test` target. The top-level one is `test: test-part1 test-part2`, and `part1/Makefile` and `part2/Makefile` each run `testius test_cases/tests.json`. The spec's Makefile section lists `make test` for part1 and part2 and omits it for the top level, but the top-level file does define it, and there is no "part1 and part2 only" option.

### Question 2

When in the terminal, in which current working directories will the `make zip` command work?

- [X] Only in the top-level `proj2-code` directory.
- [ ] Only in the `part1` subdirectory.
- [ ] Only in the `part2` subdirectory.
- [ ] This command will work in any directory.

> **Why:** Spec, "Makefile": the part1 and part2 Makefiles "do *not* support `make zip`. This is only possible in the top-level directory." Their `zip` targets just print an error telling you to change to `proj2-code`.

### Question 3

You are working on the project with a partner. How should you submit your work?

- [ ] Each partner should submit their work separately on Gradescope.
- [X] One partner should submit. After submission, they add the other partner using Gradescope's web UI.
- [ ] One partner should submit. Then you can just expect the course staff to use telepathy to determine who the other partner was.

> **Why:** Spec, "Submission": "Please only submit **one** project per group. Gradescope has an option to add group members after an assignment has been submitted."

### Question 4

Say your new `swish` shell is given a pipeline containing `n` commands to run. How many pipes should your shell create to run the pipeline?

- [ ] 0
- [ ] 1
- [X] `n - 1`
- [ ] `n`
- [ ] `n + 1`

> **Why:** Spec, Part 1: "You may safely assume that a pipeline with `n` commands must use `n-1` pipes." The figure shows seven commands and six pipes.

### Question 5

How should you represent an array of pipes in Part 2 of the project?

- [ ] As an array of `pipe_t` instances.
- [ ] As a two-dimensional array of `int`s.
- [X] As a one-dimensional array of `int`s.
- [ ] None of the above

> **Why:** Starter `swish_funcs.c`: `run_piped_command(strvec_t *tokens, int *pipes, int n_pipe_fds, int in_idx, int out_idx)`, where `pipes` is "An array of pipe file descriptors" and `n_pipe_fds` its length, indexed by `in_idx`/`out_idx`. There is no `pipe_t` type anywhere in the starter. Caveat: the question says Part 2, where the spec says all workers share one pipe; that single pipe is still `int pipe_fds[2]`, a one-dimensional `int` array, so this option holds under either reading. If this is marked wrong, the intended trick answer would be "None of the above" (no array of pipes in Part 2 at all).

### Question 6

Based on the contents of `string_vector.h`, how would I determine the number of `|` characters present in a string vector?

- [ ] `strvec_count(my_vec, "|")`
- [X] `strvec_num_occurrences(my_vec, "|")`
- [ ] `strvec_find(my_vec, "|")`
- [ ] Write a `for` loop and implement the counting logic yourself

> **Why:** `string_vector.h`: `int strvec_num_occurrences(const strvec_t *vec, const char *s)` "Returns the number of occurrences of string 's' within vector 'vec'". There is no `strvec_count`, and `strvec_find` returns only the index of the first match.

### Question 7

Say I have a `strvec_t` instance `x` containing the string sequence `"Simcoe"`, `"Bremner"`, `"York"`, `"Wellington"`, `"Harbour"`, `"King"`, and `"Oskenonton"`. I want a second `strvec_t` instance, `y`, to hold the subsequence `"York"`, `"Wellington"`, and `"Harbour"`. How do I do this?

- [ ] `strvec_copy(x, y);`
- [ ] `strvec_slice(x, y, 2, 4);`
- [ ] `strvec_slice(y, x, 2, 4);`
- [X] `strvec_slice(x, y, 2, 5);`
- [ ] `strvec_slice(y, x, 2, 5);`

> **Why:** `string_vector.h`: `int strvec_slice(const strvec_t *src, strvec_t *dest, int start, int end)`, source first, destination second, `start` inclusive, `end` exclusive. `"York"` is index 2 and `"Harbour"` is index 4, so the slice is `[2, 5)`.

### Question 8

Which of the following use cases is not covered by the provided tests in Part 1 of the project?

- [ ] A command pipeline with input and/or output redirection
- [ ] A command pipeline with three elements
- [X] A command pipeline with four or more elements
- [ ] There are no provided tests for Part 1 of the project.

> **Why:** The eight public tests in `part1/test_cases/tests.json` are two- and three-element pipelines, each with and without `<` on the first command and `>` on the last. Spec, "Hidden Tests": "the tests we have provided to you only test command pipelines of two or three programs."

### Question 9

Why do the provided tests use the `tail` command rather than the `head` command?

- [ ] There is no particular reason.
- [ ] There is no `head` command
- [X] The `head` command may not read all of its input before terminating, causing a `SIGPIPE` to be sent to its predecessor in the command pipeline.
- [ ] The `tail` command may not read all of its input before terminating, causing a `SIGPIPE` to be sent to its predecessor in the command pipeline.

> **Why:** Spec, "Hidden Tests": `head -n 10` reads "exactly ten lines of input and no more", so "its predecessor might attempt to write contents to a pipe that `head` will never consume. This may cause the predecessor program to receive a `SIGPIPE` signal". `tail` reads everything, so it is safe.

### Question 10

Why would it be safe to use `head` if we were running our command pipelines in `bash` rather than `swish`?

- [ ] Trick question: `bash` can't use `head` safely either.
- [ ] `bash` ensures that all commands in a pipeline consume all input before they can terminate.
- [ ] `bash` internally converts all uses of `head` to `tail`
- [X] `bash` does some extra signal handling around `SIGPIPE` that `swish` does not do.

> **Why:** Spec, "Hidden Tests": "the `bash` shell actually sets up its forked child processes to ignore `SIGPIPE` to suppress these kinds of error messages from occurring. Your `swish` shell is not expected to do this."

### Question 11

Say a process only writes to a pipe and never reads from that pipe. Also assume this process does not `fork()` any children. When should it close the read end of the pipe?

- [X] As soon as possible
- [ ] Only when it is done writing to the pipe
- [ ] Only when an error occurs
- [ ] Never

> **Why:** Close pipe ends you will never use immediately after `pipe()`/`fork()` (spec hint: "close inherited pipe descriptors after a `fork()` in the child process if the child will never need those descriptors"). A writer that keeps the read end open never gets `EPIPE`/`SIGPIPE` when the real reader goes away, so it can block forever on a full pipe instead of failing.

### Question 12

If we want to compute a Hadamard product in Part 2 of the project with `n` worker processes computing in parallel, how many times do we need to call `fork()`?

- [ ] `n-1` times, so that we have `n` processes in total
- [X] `n` times, since the original parent process doesn't count as a worker
- [ ] `n+1` times, to create a coordinator process and the `n` worker processes
- [ ] Trick question: We don't have to call `fork()` at all

> **Why:** Spec, "Using Processes and a Pipe": "fork one child process to serve as each worker, after setting up the pipe in the parent. The original parent process won't count as a worker." The tests say "using 1 child process", "2 child processes", and so on, for 1 and 2 workers.

### Question 13

If we want to compute a Hadamard product in Part 2 of the project with `n` worker threads computing in parallel, how many times do we need to call `pthread_create()`?

- [ ] `n-1` times, so that we have `n` threads in total
- [X] `n` times, since the original main thread doesn't count as a worker
- [ ] `n+1` times, to create a coordinator thread and the `n` worker threads
- [ ] Trick question: We don't have to call `pthread_create()` at all

> **Why:** Spec, "Using Threads": "Launch workers, this time as threads" and "In the main thread, wait for each worker thread to complete." The main thread coordinates and joins, mirroring the parent in Q12, so `n` workers means `n` calls to `pthread_create()`.

### Question 14

When using multiple processes coordinating over a pipe to compute a Hadamard product in parallel, how many worker processes need to read in files containing the two input matrices?

- [ ] One worker process, who can then share the matrix data with the others
- [X] None of the worker processes. They will already have access to the matrix data when they are created.
- [ ] All worker processes need to read in the data independently
- [ ] None of the above

> **Why:** Spec, "Using Processes and a Pipe": "*A* and *B* are read in from a file and placed on the heap of the original parent process. When the parent forks, its children will inherit copies of these heap memory regions." The starter's `worker()` takes `const int *a, const int *b`, not file names. (Its TODO comment still says "Open the two matrix data files"; that comment is stale and contradicts both the spec and the signature.)

### Question 15

What will be the arguments for the three Hadamard computation functions (using a pipe, shared memory, and threads respectively)?

- [ ] Two `char *` values giving the names of the input matrix files
- [ ] Two `char *` values giving the names of the input matrix files and an `unsigned` specifying the number of workers
- [ ] One argument of type `PyObject *`, from which we can extract the real arguments into local variables.
- [X] Two arguments of type `PyObject *`. We can extract the real arguments into local variables from the second `PyObject *` value.

> **Why:** Spec, "Making a Python Extension": "Functions called from Python accept two arguments: `PyObject *self` and `PyObject *args`. Use `PyArg_ParseTuple` to extract the arguments passed from Python into local C variables." The starter does exactly that: `PyArg_ParseTuple(args, "ssI", &mat_a_file, &mat_b_file, &n_procs)`.

### Question 16

What will the three Hadamard computation functions return back to Python after they have successfully completed their work?

- [ ] A pointer to the result matrix
- [ ] `NULL`
- [X] `None`
- [ ] Trick question: These functions have a `void` return type

> **Why:** Spec, "Making a Python Extension": "in this project we'll just return `None` back to Python on success. Accomplish this with `Py_RETURN_NONE`." The starter `hadamard_pipe()` already ends with `Py_RETURN_NONE;`.

### Question 17

What will the three Hadamard computation functions return if they encounter an error and need Python to raise an exception?

- [ ] `None`
- [X] `NULL`
- [ ] `PyExc_OSError`
- [ ] None of the above

> **Why:** Spec, "Making a Python Extension": "If an error occurs, your function should instead return `NULL`. Before doing so, however, it should set up a **Python error**" (with `PyErr_SetFromErrno` or `PyErr_SetString`; `PyExc_OSError` is the exception *type* you pass, not the return value). The starter returns `NULL` after a failed `PyArg_ParseTuple` or `read_matrix_from_file`.

### Question 18

If no errors occur, what does each Hadamard computation function need to do before it returns to pass the test cases?

- [ ] Trick question: There is nothing extra to do before returning
- [ ] Set `errno` to `0`
- [X] Call the `print_matrix()` function with the result matrix
- [ ] Call the `print_matrix()` function on the two input matrices and the result matrix

> **Why:** Spec, "Utility Functions": "`print_matrix`, should be called by each of your Hadamard product functions once the result matrix has been computed ... so that the output of your program matches what's expected in the tests." The expected output for a 128x128 input (`part2/test_cases/output/mat_00.txt`) is exactly 128 lines, so only the result matrix is printed.
