# CSCI 4061 -- Quiz 07

> Archived from the Canvas quiz page
> <https://canvas.umn.edu/courses/579183/quizzes/1253423>
>
> | | |
> |---|---|
> | **Due** | Mon Nov 2, 2026 11:59pm |
> | **Points** | 10 (10 questions, 1 pt each) |
> | **Available** | Oct 26, 2026 12:00am - Nov 2, 2026 11:59pm |
> | **Allowed attempts** | 10 |
> | **Time limit** | none |

---

## Quiz Instructions

This quiz focuses on course content introduced in the lectures of Week 7, the reading assignments associated with these lectures, and Lab 7. To prepare for this quiz, make sure you are familiar with these materials, and review them as necessary.

Credit is earned for this quiz when you submit your answers. You may resubmit up to ten times before the quiz deadline (usually one week after the quiz's release date).

## Questions

### Question 1

Which of the following is *not* shared among the multiple threads of a process?

- [ ] Stack
- [ ] Heap
- [ ] Globals
- [ ] Code

### Question 2

What kind of workload would still benefit from using multiple threads, even on a computer with a single-core CPU?

- [ ] Work involving lots of computation, such as approximating the value of pi
- [ ] Work that requires a lot of storage in memory, like analyzing a large dataset
- [ ] Work that involves lots of blocking operations, like reading from files, pipes, and sockets
- [ ] Trick question: Multithreading is only useful on multi-core systems

### Question 3

All threads start their life by calling a function. Which of the following is an example of a correctly-typed thread start function?

- [ ] `void do_work()`
- [ ] `void do_work(void *arg)`
- [ ] `void *do_work()`
- [ ] `void *do_work(void *arg)`

### Question 4

Why do we have to print out an error from a call to a `pthread` function manually, with `fprintf()` and `strerror()` rather than with `perror()`?

- [ ] This isn't required for thread calls, it's just the preferred convention.
- [ ] `perror()` is not safe for concurrent use by multiple threads - the output on the screen could end up garbled.
- [ ] `pthread` functions don't set the `errno` global variable
- [ ] Threads are not allowed to print to `stderr`, only to `stdout`, so we need to use `fprintf()` to ensure that the error message goes to the correct destination.

### Question 5

Which of the following lines of C code is most likely to be atomic? Assume `p` is of type `int *` and is not `NULL`, and assume `x` is of type `int`.

- [ ] `*p++;`
- [ ] `*p = *p + 5;`
- [ ] `x = *p + 5;`
- [ ] `x = 5;`

### Question 6

Which of the following snippets of C code is mostly like to be atomic (at least from the perspective of all threads belonging to the current process)?

Assume `p` is of type `int *` and is not `NULL`, assume `x` is of type `int`, and assume `lock` is of type `pthread_mutex_t` and has been properly initialized.

You may also assume that no errors occur.

- [ ] `pthread_mutex_lock(&lock); *p++; pthread_mutex_unlock(&lock);`
- [ ] `pthread_mutex_lock(&lock); x = 4; pthread_mutex_unlock(&lock);`
- [ ] `pthread_mutex_lock(&lock); x = *p; x = x + 15; *p = x; pthread_mutex_unlock(&lock);`
- [ ] All of the Above

### Question 7

Which of the following is *not* a problem with busy waiting?

- [ ] It wastes CPU cycles
- [ ] It wastes space in memory
- [ ] It prevents other threads from getting a chance to run
- [ ] If we're unlucky, the condition the current thread is waiting on never becomes true

### Question 8

What is wrong with the following code for removing an element from a thread-safe bounded queue?

``` c
if (queue->size == 0) {
    pthread_cond_wait(&queue->cv_empty, &queue->lock);
}
// Remove element from queue
pthread_mutex_unlock(queue->lock);
pthread_cond_signal(&queue->cv_full);
```

- [ ] A thread needs to lock `queue->lock` before checking if the queue is empty.
- [ ] A thread should check for a condition it may need to wait on using `while` rather than `if`.
- [ ] A thread should hold the queue's lock when it signals one of the data structure's condition variables.
- [ ] All of the above

### Question 9

What is wrong with the following code for removing an element from a thread-safe bounded queue?

``` c
pthread_mutex_lock(&queue->lock);
while (queue->size == 0) {
    pthread_cond_wait(&queue->cv_empty, &queue->lock);
}
// Remove element from queue
pthread_cond_signal(&queue->cv_empty);
pthread_mutex_unlock(queue->lock);
```

- [ ] A thread doesn't need to acquire the queue's lock before calling `pthread_cond_wait()`.
- [ ] A thread should unlock the queue's lock before calling `pthread_cond_signal()`.
- [ ] The code is not signaling the correct condition variable.
- [ ] None of the above

### Question 10

Which of the following is true about `pthread_cond_signal()`?

- [ ] The signaled thread will be given a turn on the CPU immediately.
- [ ] This function will notify all threads waiting on the condition variable.
- [ ] If no thread is currently waiting on the condition variable, this notification is thrown away.
- [ ] It is best practice not to hold the relevant lock when signaling a condition variable.
