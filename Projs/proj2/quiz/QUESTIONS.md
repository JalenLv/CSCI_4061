# CSCI 4061 -- Project 2 Quiz

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

---

## Quiz Instructions

This quiz will cover some of the important elements of the Project 2 assignment specification and starter code. Start by reading the full [Project 2 Assignment](https://canvas.umn.edu/courses/579183/assignments/5444400) spec carefully. You should also look over the project starter code, particularly any header (`.h`) files.

## Questions

### Question 1

When in the terminal and working on Project 2, in which current working directories will the `make test` command work?

- [ ] You can run `make test` in the `part1` subdirectory to run tests for Part 1 of the project.
- [ ] You can run `make test` in the `part2` subdirectory to run tests for Part 2 of the project.
- [ ] You can run `make test` in the top-level `proj2-code` directory to run tests for both parts of the project.
- [ ] All of the above

### Question 2

When in the terminal, in which current working directories will the `make zip` command work?

- [ ] Only in the top-level `proj2-code` directory.
- [ ] Only in the `part1` subdirectory.
- [ ] Only in the `part2` subdirectory.
- [ ] This command will work in any directory.

### Question 3

You are working on the project with a partner. How should you submit your work?

- [ ] Each partner should submit their work separately on Gradescope.
- [ ] One partner should submit. After submission, they add the other partner using Gradescope's web UI.
- [ ] One partner should submit. Then you can just expect the course staff to use telepathy to determine who the other partner was.

### Question 4

Say your new `swish` shell is given a pipeline containing `n` commands to run. How many pipes should your shell create to run the pipeline?

- [ ] 0
- [ ] 1
- [ ] `n - 1`
- [ ] `n`
- [ ] `n + 1`

### Question 5

How should you represent an array of pipes in Part 2 of the project?

- [ ] As an array of `pipe_t` instances.
- [ ] As a two-dimensional array of `int`s.
- [ ] As a one-dimensional array of `int`s.
- [ ] None of the above

### Question 6

Based on the contents of `string_vector.h`, how would I determine the number of `|` characters present in a string vector?

- [ ] `strvec_count(my_vec, "|")`
- [ ] `strvec_num_occurrences(my_vec, "|")`
- [ ] `strvec_find(my_vec, "|")`
- [ ] Write a `for` loop and implement the counting logic yourself

### Question 7

Say I have a `strvec_t` instance `x` containing the string sequence `"Simcoe"`, `"Bremner"`, `"York"`, `"Wellington"`, `"Harbour"`, `"King"`, and `"Oskenonton"`. I want a second `strvec_t` instance, `y`, to hold the subsequence `"York"`, `"Wellington"`, and `"Harbour"`. How do I do this?

- [ ] `strvec_copy(x, y);`
- [ ] `strvec_slice(x, y, 2, 4);`
- [ ] `strvec_slice(y, x, 2, 4);`
- [ ] `strvec_slice(x, y, 2, 5);`
- [ ] `strvec_slice(y, x, 2, 5);`

### Question 8

Which of the following use cases is not covered by the provided tests in Part 1 of the project?

- [ ] A command pipeline with input and/or output redirection
- [ ] A command pipeline with three elements
- [ ] A command pipeline with four or more elements
- [ ] There are no provided tests for Part 1 of the project.

### Question 9

Why do the provided tests use the `tail` command rather than the `head` command?

- [ ] There is no particular reason.
- [ ] There is no `head` command
- [ ] The `head` command may not read all of its input before terminating, causing a `SIGPIPE` to be sent to its predecessor in the command pipeline.
- [ ] The `tail` command may not read all of its input before terminating, causing a `SIGPIPE` to be sent to its predecessor in the command pipeline.

### Question 10

Why would it be safe to use `head` if we were running our command pipelines in `bash` rather than `swish`?

- [ ] Trick question: `bash` can't use `head` safely either.
- [ ] `bash` ensures that all commands in a pipeline consume all input before they can terminate.
- [ ] `bash` internally converts all uses of `head` to `tail`
- [ ] `bash` does some extra signal handling around `SIGPIPE` that `swish` does not do.

### Question 11

Say a process only writes to a pipe and never reads from that pipe. Also assume this process does not `fork()` any children. When should it close the read end of the pipe?

- [ ] As soon as possible
- [ ] Only when it is done writing to the pipe
- [ ] Only when an error occurs
- [ ] Never

### Question 12

If we want to compute a Hadamard product in Part 2 of the project with `n` worker processes computing in parallel, how many times do we need to call `fork()`?

- [ ] `n-1` times, so that we have `n` processes in total
- [ ] `n` times, since the original parent process doesn't count as a worker
- [ ] `n+1` times, to create a coordinator process and the `n` worker processes
- [ ] Trick question: We don't have to call `fork()` at all

### Question 13

If we want to compute a Hadamard product in Part 2 of the project with `n` worker threads computing in parallel, how many times do we need to call `pthread_create()`?

- [ ] `n-1` times, so that we have `n` threads in total
- [ ] `n` times, since the original main thread doesn't count as a worker
- [ ] `n+1` times, to create a coordinator thread and the `n` worker threads
- [ ] Trick question: We don't have to call `pthread_create()` at all

### Question 14

When using multiple processes coordinating over a pipe to compute a Hadamard product in parallel, how many worker processes need to read in files containing the two input matrices?

- [ ] One worker process, who can then share the matrix data with the others
- [ ] None of the worker processes. They will already have access to the matrix data when they are created.
- [ ] All worker processes need to read in the data independently
- [ ] None of the above

### Question 15

What will be the arguments for the three Hadamard computation functions (using a pipe, shared memory, and threads respectively)?

- [ ] Two `char *` values giving the names of the input matrix files
- [ ] Two `char *` values giving the names of the input matrix files and an `unsigned` specifying the number of workers
- [ ] One argument of type `PyObject *`, from which we can extract the real arguments into local variables.
- [ ] Two arguments of type `PyObject *`. We can extract the real arguments into local variables from the second `PyObject *` value.

### Question 16

What will the three Hadamard computation functions return back to Python after they have successfully completed their work?

- [ ] A pointer to the result matrix
- [ ] `NULL`
- [ ] `None`
- [ ] Trick question: These functions have a `void` return type

### Question 17

What will the three Hadamard computation functions return if they encounter an error and need Python to raise an exception?

- [ ] `None`
- [ ] `NULL`
- [ ] `PyExc_OSError`
- [ ] None of the above

### Question 18

If no errors occur, what does each Hadamard computation function need to do before it returns to pass the test cases?

- [ ] Trick question: There is nothing extra to do before returning
- [ ] Set `errno` to `0`
- [ ] Call the `print_matrix()` function with the result matrix
- [ ] Call the `print_matrix()` function on the two input matrices and the result matrix
