# CSCI 4061 -- Quiz 06

> Archived from the Canvas quiz page
> <https://canvas.umn.edu/courses/579183/quizzes/1252321>
>
> | | |
> |---|---|
> | **Due** | Mon Oct 26, 2026 11:59pm |
> | **Points** | 10 (10 questions, 1 pt each) |
> | **Available** | Oct 19, 2026 12:00am - Oct 26, 2026 11:59pm |
> | **Allowed attempts** | 10 |
> | **Time limit** | none |

---

## Quiz Instructions

This quiz focuses on course content introduced in the lectures of Week 6, the reading assignments associated with these lectures, and Lab 5. To prepare for this quiz, make sure you are familiar with these materials, and review them as necessary.

Credit is earned for this quiz when you submit your answers. You may resubmit up to ten times before the quiz deadline (usually one week after the quiz's release date).

## Questions

### Question 1

Say we have a properly initialized array of `struct pollfd` elements of length `n` called `fds`. When will the system call `poll(fds, n, 10)` return?

- [ ] It will return immediately, after checking the status of each referenced file descriptor.
- [ ] It will return when at least one of the referenced file descriptors has a pending event.
- [ ] It will return when at least one of the referenced file descriptors has a pending event *or* after a timeout of 10 milliseconds expires, if no events have yet occurred.
- [ ] None of the above

### Question 2

Say we have a properly initialized array of `struct pollfd` elements of length `n` called `fds`. When will the system call `poll(fds, n, 0)` return?

- [ ] It will return immediately, after checking the status of each referenced file descriptor.
- [ ] It will return when at least one of the referenced file descriptors has a pending event.
- [ ] It will return when at least one of the referenced file descriptors has a pending event *or* after a timeout of 10 milliseconds expires, if no events have yet occurred.
- [ ] None of the above

### Question 3

Say we have a properly initialized array of `struct pollfd` elements of length `n` called `fds`. When will the system call `poll(fds, n, -1)` return?

- [ ] It will return immediately, after checking the status of each referenced file descriptor.
- [ ] It will return when at least one of the referenced file descriptors has a pending event.
- [ ] It will return when at least one of the referenced file descriptors has a pending event *or* after a timeout of 10 milliseconds expires, if no events have yet occurred.
- [ ] None of the above

### Question 4

Say one of the file descriptors passed to a call to `poll` represents the read end of a pipe. When will `poll` set the `POLLHUP` field in the descriptor's corresponding `revents` field?

- [ ] When data can be read from the pipe without blocking
- [ ] When data can be written to the pipe without blocking
- [ ] When all processes have closed the read end of the pipe
- [ ] When all processes have closed the write end of the pipe

### Question 5

We can instruct `poll` to ignore one of the `struct fd` instances passed to it by setting the struct's `fd` field to what value?

- [ ] `0`
- [ ] `POLLIGNORE`
- [ ] `-1`
- [ ] Any negative number

### Question 6

Say we have two Python variables `x` and `y` holding numerical values and we want to convert them to a sequence of bytes that represent a C `int` value followed by a C `unsigned int` value. Which of the following would accomplish this?

- [ ] `struct.pack("iI", x, y)`
- [ ] `struct.pack("Ii", x, y)`
- [ ] `struct.unpack("iI", x, y)`
- [ ] `struct.unpack("Ii", x, y)`

### Question 7

What is a Python feature that is helpful for ensuring resources get cleaned up (e.g., a file is closed) in both success and error situations?

- [ ] The `with` keyword
- [ ] The `finally` keyword
- [ ] Both `with` and `finally`
- [ ] The `cleanup` keyword

### Question 8

What happens in a C extension function for Python when `PyArg_ParseTuple` is unable to extract and convert to C values of the specific types from a function's Python arguments?

- [ ] It returns `false`
- [ ] It returns `-1`.
- [ ] It specifies that a `TypeError` should be raised once the C extension function returns `NULL`.
- [ ] It specifies that a `TypeError` should be raised on a `NULL` return from the extension function and it returns `false`.

### Question 9

What is the reference count of a new Python list created within a C extension function with `PyList_New`?

- [ ] `0`
- [ ] `1`
- [ ] You need to specify the reference count as an argument to `PyList_New`
- [ ] None of the above

### Question 10

Which of the following functions is used to instruct the Python interpreter to raise an `OSError` with a message determined from the value of `errno`?

- [ ] `PyErr_Format(PyExc_OSError, "errno");`
- [ ] `PyErr_SetString(PyExc_OSError, "errno");`
- [ ] `PyErr_SetFromErrno(PyExc_OSError);`
- [ ] `PyErr_SetFromErrno(PyExc_ValueError);`
