# CSCI 4061 -- Quiz 06 (answered)

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

> **Note:** Answered copy of `QUESTIONS.md`; every choice cites the Week 6 slides, Lec10 *More Shared Mem* (Oct 13) and Lec11 *Python Systems Programming* (Oct 15). Q5 isn't on the slides and cites poll(2) instead. Q1-Q9 were also checked with small programs in the devcontainer. Q5 carries a caveat.

---

## Quiz Instructions

This quiz focuses on course content introduced in the lectures of Week 6, the reading assignments associated with these lectures, and Lab 5. To prepare for this quiz, make sure you are familiar with these materials, and review them as necessary.

Credit is earned for this quiz when you submit your answers. You may resubmit up to ten times before the quiz deadline (usually one week after the quiz's release date).

## Questions

### Question 1

Say we have a properly initialized array of `struct pollfd` elements of length `n` called `fds`. When will the system call `poll(fds, n, 10)` return?

- [ ] It will return immediately, after checking the status of each referenced file descriptor.
- [ ] It will return when at least one of the referenced file descriptors has a pending event.
- [X] It will return when at least one of the referenced file descriptors has a pending event *or* after a timeout of 10 milliseconds expires, if no events have yet occurred.
- [ ] None of the above

> **Why:** Lec10 slide 22: `timeout` is "How long to wait until giving up and moving on", and `poll()` returns the "Number of file descriptors with pending events". The unit is milliseconds: poll(2) says "The timeout argument specifies the number of milliseconds that poll() should block", and slide 25 passes `1000` to "block for up to one second". Tested on a pipe whose writer stayed silent: `poll(fds, 1, 10)` returned 0 after 10.1 ms.

### Question 2

Say we have a properly initialized array of `struct pollfd` elements of length `n` called `fds`. When will the system call `poll(fds, n, 0)` return?

- [X] It will return immediately, after checking the status of each referenced file descriptor.
- [ ] It will return when at least one of the referenced file descriptors has a pending event.
- [ ] It will return when at least one of the referenced file descriptors has a pending event *or* after a timeout of 10 milliseconds expires, if no events have yet occurred.
- [ ] None of the above

> **Why:** Lec10 slide 22: a timeout of "0: Don't wait at all, just check if events already pending". In the same test, `poll(fds, 1, 0)` returned 0 after 0.0 ms.

### Question 3

Say we have a properly initialized array of `struct pollfd` elements of length `n` called `fds`. When will the system call `poll(fds, n, -1)` return?

- [ ] It will return immediately, after checking the status of each referenced file descriptor.
- [X] It will return when at least one of the referenced file descriptors has a pending event.
- [ ] It will return when at least one of the referenced file descriptors has a pending event *or* after a timeout of 10 milliseconds expires, if no events have yet occurred.
- [ ] None of the above

> **Why:** Lec10 slide 22: a timeout of "-1: Wait as long as necessary, never quit early". In the same test, `poll(fds, 1, -1)` returned 1 with `POLLIN` set after 290 ms, when the child wrote a byte.

### Question 4

Say one of the file descriptors passed to a call to `poll` represents the read end of a pipe. When will `poll` set the `POLLHUP` field in the descriptor's corresponding `revents` field?

- [ ] When data can be read from the pipe without blocking
- [ ] When data can be written to the pipe without blocking
- [ ] When all processes have closed the read end of the pipe
- [X] When all processes have closed the write end of the pipe

> **Why:** Lec10 slide 27: "POLLHUP triggered as soon as write end of pipe is closed by other process"; slide 24 describes it as "'Device' has been disconnected (e.g., by a pipe close())". Tested: while a child still held the write end, `poll()` on the drained read end timed out with no events; once the child closed it, `poll()` returned `POLLHUP`. The first two choices describe `POLLIN` and `POLLOUT` (slide 24). A closed read end is reported to the writer instead: poll(2) sets `POLLERR` "for a file descriptor referring to the write end of a pipe when the read end has been closed".

### Question 5

We can instruct `poll` to ignore one of the `struct fd` instances passed to it by setting the struct's `fd` field to what value?

- [ ] `0`
- [ ] `POLLIGNORE`
- [ ] `-1`
- [X] Any negative number

> **Why:** Not on the slides. poll(2): "If this field is negative, then the corresponding events field is ignored and the revents field returns zero. (This provides an easy way of ignoring a file descriptor for a single poll() call: simply negate the fd field.)" Tested: entries with `fd` set to `-1` and to `-7` were both ignored (`poll()` returned 0 at the timeout with `revents` 0), while an unopened fd 99 came back as `POLLNVAL`. Caveat: `-1` (choice 3) works too, so the key could be either, but choice 4 is the rule the man page states. `0` is stdin, and no header defines `POLLIGNORE`. The question's `struct fd` means `struct pollfd`.

### Question 6

Say we have two Python variables `x` and `y` holding numerical values and we want to convert them to a sequence of bytes that represent a C `int` value followed by a C `unsigned int` value. Which of the following would accomplish this?

- [X] `struct.pack("iI", x, y)`
- [ ] `struct.pack("Ii", x, y)`
- [ ] `struct.unpack("iI", x, y)`
- [ ] `struct.unpack("Ii", x, y)`

> **Why:** Lec11 slide 11: format `i` is a C `int` and `I` an `unsigned int`; `struct.pack` converts to bytes and `struct.unpack` converts from bytes. So the format is `"iI"` and the function is `pack`. Tested with `x = -1` and `y = 2**32 - 1`: `struct.pack("iI", x, y)` gave 8 bytes that unpack back to `(-1, 4294967295)`, `struct.pack("Ii", x, y)` raised `struct.error: argument out of range`, and `struct.unpack("iI", x, y)` raised `TypeError` (it takes a format and one bytes object).

### Question 7

What is a Python feature that is helpful for ensuring resources get cleaned up (e.g., a file is closed) in both success and error situations?

- [ ] The `with` keyword
- [ ] The `finally` keyword
- [X] Both `with` and `finally`
- [ ] The `cleanup` keyword

> **Why:** Lec11 slide 12: a `finally:` block runs "on success or exception", and slide 8's child closes its pipe end in `finally: os.close(w)`. The slides don't show `with`, but Python runs a `with` block's context-manager `__exit__()` however the block is left, and a file's `__exit__()` closes it. Tested: a file opened with `with open(...) as f:` was closed (`f.closed` was `True`) after an exception propagated out of the block. Python has no `cleanup` keyword.

### Question 8

What happens in a C extension function for Python when `PyArg_ParseTuple` is unable to extract and convert to C values of the specific types from a function's Python arguments?

- [ ] It returns `false`
- [ ] It returns `-1`.
- [ ] It specifies that a `TypeError` should be raised once the C extension function returns `NULL`.
- [X] It specifies that a `TypeError` should be raised on a `NULL` return from the extension function and it returns `false`.

> **Why:** Lec11 slide 21: when an extension function returns `NULL`, "Exception is raised by interpreter", and slide 24 adds that "C code must specify which exception to raise before returning". On failure `PyArg_ParseTuple` does both halves itself: it sets the exception and returns false. The lab06 starter relies on this with `if (!PyArg_ParseTuple(args, "O!", &PyList_Type, &arg_list)) return NULL;` (`square_root.c` line 37). Tested by passing a `str` where format `"i"` wanted an int: it returned 0 with a `TypeError` pending, and returning `NULL` made Python raise `TypeError: 'str' object cannot be interpreted as an integer`. Choice 1 is true but leaves out the exception, choice 3 leaves out the return value, and it never returns `-1`.

### Question 9

What is the reference count of a new Python list created within a C extension function with `PyList_New`?

- [ ] `0`
- [X] `1`
- [ ] You need to specify the reference count as an argument to `PyList_New`
- [ ] None of the above

> **Why:** Lec11 slide 30: "PyList_New returns new object with reference count 1", and "Our C code is considered the owner of this reference until list is returned". Lab 06 slide 8 says the same. Tested: `Py_REFCNT(PyList_New(3))` printed 1. `PyList_New` takes only the length (slide 28).

### Question 10

Which of the following functions is used to instruct the Python interpreter to raise an `OSError` with a message determined from the value of `errno`?

- [ ] `PyErr_Format(PyExc_OSError, "errno");`
- [ ] `PyErr_SetString(PyExc_OSError, "errno");`
- [X] `PyErr_SetFromErrno(PyExc_OSError);`
- [ ] `PyErr_SetFromErrno(PyExc_ValueError);`

> **Why:** Lec11 slide 26: "Using value of errno set by a failed C system call to determine msg: PyErr_SetFromErrno(PyExc_OSError);", repeated on Lab 06 slide 10. Choices 1 and 2 would raise `OSError` with the literal message "errno" (slides 24-25 show `PyErr_SetString` and `PyErr_Format` taking the message text), and choice 4 raises a `ValueError`.
