# CSCI 4061 -- Quiz 07 (answered)

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

> **Note:** Answered copy of `QUESTIONS.md`; every choice cites the Week 7 slides, Lec12 *Threads* (Oct 20) and Lec13 *Thread Synchronization* (Oct 22). Q3 and Q5 were also checked by compiling them in the devcontainer.

---

## Quiz Instructions

This quiz focuses on course content introduced in the lectures of Week 7, the reading assignments associated with these lectures, and Lab 7. To prepare for this quiz, make sure you are familiar with these materials, and review them as necessary.

Credit is earned for this quiz when you submit your answers. You may resubmit up to ten times before the quiz deadline (usually one week after the quiz's release date).

## Questions

### Question 1

Which of the following is *not* shared among the multiple threads of a process?

- [X] Stack
- [ ] Heap
- [ ] Globals
- [ ] Code

> **Why:** Lec12 slide 11: "Each thread needs its own stack and CPU register values", while "Threads in the same process share everything else": "Heap, Code, Data" and "Open Files, Signal Settings". Slide 12's diagram draws one stack per thread next to a single shared data, code and heap.

### Question 2

What kind of workload would still benefit from using multiple threads, even on a computer with a single-core CPU?

- [ ] Work involving lots of computation, such as approximating the value of pi
- [ ] Work that requires a lot of storage in memory, like analyzing a large dataset
- [X] Work that involves lots of blocking operations, like reading from files, pipes, and sockets
- [ ] Trick question: Multithreading is only useful on multi-core systems

> **Why:** Lec12 slide 18, the first of two motivations for threads: "Concurrent task execution: When one thread blocks (e.g., read() from a pipe or a network socket) another thread from the same process can run", which "doesn't require a multi-core CPU". Slide 14 makes the same point: threads came before multi-core CPUs. Speeding up computation is the second motivation, parallel execution, which slide 17 says "Requires a multi-core CPU". Neither of the first two workloads blocks, so a single core has nothing to overlap.

### Question 3

All threads start their life by calling a function. Which of the following is an example of a correctly-typed thread start function?

- [ ] `void do_work()`
- [ ] `void do_work(void *arg)`
- [ ] `void *do_work()`
- [X] `void *do_work(void *arg)`

> **Why:** Lec12 slide 21: `pthread_create()` takes `void *(*start_routine) (void *)`, and slide 22 says "a thread function only takes a single void * argument" and "can only return a single void * value". Compiled each choice as the third argument to `pthread_create()` with `-Wall -Werror` (gcc 11): both `void` return types fail with "incompatible pointer type". `void *do_work()` compiles too, because before C23 empty parentheses leave the parameters unspecified, but it doesn't declare the `void *` parameter, so only choice 4 has the right type.

### Question 4

Why do we have to print out an error from a call to a `pthread` function manually, with `fprintf()` and `strerror()` rather than with `perror()`?

- [ ] This isn't required for thread calls, it's just the preferred convention.
- [ ] `perror()` is not safe for concurrent use by multiple threads - the output on the screen could end up garbled.
- [X] `pthread` functions don't set the `errno` global variable
- [ ] Threads are not allowed to print to `stderr`, only to `stdout`, so we need to use `fprintf()` to ensure that the error message goes to the correct destination.

> **Why:** Lec12 slide 24: "The pthread functions don't use errno. Instead, they return an 'error number'. Use strerror() to convert the error number to a meaningful string. This is actually what perror() does for you before it prints". Slide 25 shows the pattern: `fprintf(stderr, "pthread_create failed: %s\n", strerror(result));`. `perror()` reads `errno`, which the `pthread` call never set.

### Question 5

Which of the following lines of C code is most likely to be atomic? Assume `p` is of type `int *` and is not `NULL`, and assume `x` is of type `int`.

- [ ] `*p++;`
- [ ] `*p = *p + 5;`
- [ ] `x = *p + 5;`
- [X] `x = 5;`

> **Why:** Lec13 slide 4: "A line of C code may appear atomic when it is not"; `*sum += 1;` is three instructions (load, increment, store), and slides 5-10 show two threads interleaving them so one update is lost. Compiled the four lines with globals `x` and `p` (`gcc -O0 -S`): `x = 5;` is the single instruction `movl $5, x(%rip)`. `*p++;` parses as `*(p++)` and is a load, add and store of `p`, `*p = *p + 5;` is five instructions, and `x = *p + 5;` is four.

### Question 6

Which of the following snippets of C code is mostly like to be atomic (at least from the perspective of all threads belonging to the current process)?

Assume `p` is of type `int *` and is not `NULL`, assume `x` is of type `int`, and assume `lock` is of type `pthread_mutex_t` and has been properly initialized.

You may also assume that no errors occur.

- [ ] `pthread_mutex_lock(&lock); *p++; pthread_mutex_unlock(&lock);`
- [ ] `pthread_mutex_lock(&lock); x = 4; pthread_mutex_unlock(&lock);`
- [ ] `pthread_mutex_lock(&lock); x = *p; x = x + 15; *p = x; pthread_mutex_unlock(&lock);`
- [X] All of the Above

> **Why:** Lec13 slide 13: a critical section "needs to occur from start to finish atomically (at least from perspective of the current process)", and "Only one thread should be in the middle of a critical section at a time -> critical section appears atomic to all other threads". All three snippets do their work between `pthread_mutex_lock(&lock)` and `pthread_mutex_unlock(&lock)`, the same fix slide 20 applies to `*sum += elements[i]`. This holds only if every other thread that touches `*p` or `x` takes `lock` too.

### Question 7

Which of the following is *not* a problem with busy waiting?

- [ ] It wastes CPU cycles
- [X] It wastes space in memory
- [ ] It prevents other threads from getting a chance to run
- [ ] If we're unlucky, the condition the current thread is waiting on never becomes true

> **Why:** Lec13 slide 26 lists the problems with busy waiting: it "Wastes CPU cycles doing useless work", it "Prevents other threads from running", and "If you're unlucky, this thread is only preempted while it holds the lock, so no other thread can even change the shared data!", so the condition never becomes true. Memory isn't on the list: the loop repeats a check and allocates nothing.

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
- [X] All of the above

> **Why:** Compare Lec13 slide 32, "Proper Dequeue", which avoids all three problems. It calls `pthread_mutex_lock(&lock)` before checking `queue->size`; `pthread_cond_wait()` needs that too, since it "atomically releases lock" (slide 29). It waits in a `while` loop because of "spurious wakeups" and because another consumer can take the item before the woken thread runs (slide 33). And it signals before unlocking: slide 29 says "Should only call this when current thread holds relevant mutex". (`pthread_mutex_unlock(queue->lock)` is missing its `&` here and in Q9; that's a typo in the question, not one of the choices.)

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
- [X] The code is not signaling the correct condition variable.
- [ ] None of the above

> **Why:** Lec13 slide 32: after removing an item, Proper Dequeue calls `pthread_cond_signal(&full_cv)`, to "Send notification in case any thread is waiting for non-full queue". This code signals `cv_empty`, which only consumers wait on, so a producer blocked on a full queue is never woken. The first two choices contradict slide 29: `pthread_cond_wait()` releases a lock the caller must already hold, and you should signal only "when current thread holds relevant mutex".

### Question 10

Which of the following is true about `pthread_cond_signal()`?

- [ ] The signaled thread will be given a turn on the CPU immediately.
- [ ] This function will notify all threads waiting on the condition variable.
- [X] If no thread is currently waiting on the condition variable, this notification is thrown away.
- [ ] It is best practice not to hold the relevant lock when signaling a condition variable.

> **Why:** Lec13 slide 29: "If no thread waiting, notification is thrown away (they don't persist)". The other choices contradict the slides: a notified thread "becomes ready to run but does not necessarily run right away" (slide 33), notifying all waiters is `pthread_cond_broadcast()` (slide 29), and slide 29 says to signal only "when current thread holds relevant mutex".
