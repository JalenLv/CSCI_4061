# CSCI 4061 -- Quiz 08 (answered)

> Archived from the Canvas quiz page
> <https://canvas.umn.edu/courses/579183/quizzes/1254521>
>
> | | |
> |---|---|
> | **Due** | Mon Nov 9, 2026 11:59pm |
> | **Points** | 10 (10 questions, 1 pt each) |
> | **Available** | Nov 2, 2026 12:00am - Nov 9, 2026 11:59pm |
> | **Allowed attempts** | 10 |
> | **Time limit** | none |

> **Note:** Answered copy of `QUESTIONS.md`; every choice cites the Week 8 slides, Lec14 *More Synchronization* (Oct 27) and Lec15 *Synchronization, Networking* (Oct 29). Q5 and Q6 were also tested with small C programs. Q6 carries a caveat.

---

## Quiz Instructions

This quiz focuses on course content introduced in the lectures of Week 8 , the reading assignments associated with these lectures, and Lab 8. To prepare for this quiz, make sure you are familiar with these materials, and review them as necessary.

Credit is earned for this quiz when you submit your answers. You may resubmit up to ten times before the quiz deadline (usually one week after the quiz's release date).

## Questions

### Question 1

One recommendation when writing multithreaded code is that all threads acquire a group of locks in the same order. For example, to acquire locks `a` and `b`, *all* threads might use the following code:

``` c
pthread_mutex_lock(&a);
pthread_mutex_lock(&b);
```

Which of the four classical requirements for deadlock does this requirement help us avoid?

- [ ] Mutual Exclusion
- [ ] Hold and Wait
- [ ] No Preemption
- [X] Circular Wait

> **Why:** Lec14 slides 7-8 list the four conditions, including "Circular Wait: Dependencies of each thread create a cycle", next to the "Good convention to follow: All threads try to acquire locks in the same order". Slide 7, where Thread 2 locks `b` then `a`, asks "What can go wrong?"; slide 8, where both lock `a` then `b`, is the "Better Version". Slide 5 uses the same fix for the dining philosophers, where one philosopher takes the other chopstick first, which "breaks cycle that would cause deadlock". The other three conditions still hold under lock ordering.

### Question 2

Say we have a binary tree data structure shared among multiple threads and use a `pthread_rwlock_t` to synchronize access to the tree. Which of the following operations would require a call to `pthread_rwlock_wrlock()`?

- [ ] Adding an Element to the Tree
- [ ] Deleting an Element from the Tree
- [ ] Rearranging the Tree so that it is Balanced
- [X] All of the Above

> **Why:** Lec14 slide 12: a write lock proceeds only "as long as no thread holds read lock and no thread holds write lock". Slide 10's exercise table shows why that's needed: operations that change the structure (enqueue, dequeue) can't safely run alongside anything. Adding, deleting and rebalancing all change the tree.

### Question 3

For the binary tree scenario described above, which of the following operations would require a call to `pthread_rwlock_rdlock()`?

- [X] Searching the Tree
- [ ] Adding an element to the Tree
- [ ] Deleting an Element from the Tree
- [ ] Rearranging the Tree so that it is Balanced

> **Why:** Lec14 slide 12: with a read lock the "Calling thread can proceed as long as no thread holds write lock" and "Multiple threads may simultaneously hold read locks", so a read lock is only safe for operations that don't modify the tree. Searching is the only one.

### Question 4

If we consider the performance of all threads in aggregate, which operation will benefit the most from the use of a `pthread_rwlock_t` instead of a `pthread_mutex_t` to synchronize access to the binary tree?

- [X] Searching the Tree
- [ ] Adding an element to the Tree
- [ ] Deleting an Element from the Tree
- [ ] Rearranging the Tree so that it is Balanced

> **Why:** Lec14 slide 12: readers share the lock; writers exclude everyone. Under a mutex every search runs alone; under a read-write lock searches run in parallel. That is the point of slide 10's table, where only the read-only pairs (length, print) are marked safe to run together. Writes are exclusive under either lock, so they gain nothing.

### Question 5

Consider a thread-safe queue of bounded size with thread synchronization implemented using semaphores. In lecture, we saw that the beginning of the `enqueue()` operation had the following sequence of operations:

``` c
sem_wait(&queue->slots);
sem_wait(&queue->lock);
```

Why is this better than doing these operations in the opposite order, like so?

``` c
sem_wait(&queue->lock);
sem_wait(&queue->slots);
```

- [ ] Actually, neither order is better than the other.
- [ ] The first order may lead to better performance than the second order.
- [X] The second order can induce a deadlock while the first cannot.
- [ ] None of the above

> **Why:** Lec14 slide 21's `queue_add` uses the first order. Reproduced with a capacity-1 version of that queue (producer enqueues 3 items, consumer dequeues 3; 3 runs of each order): the first order finished every run, the second hung every run. With the second order, a producer that finds the queue full holds `lock` while blocked in `sem_wait(&queue->slots)`. Only a remove can free a slot, and it needs `lock` first. The first order never blocks while holding `lock`.

### Question 6

Consider the same queue data structure described in the previous question. In lecture, we saw that the end of the `enqueue()` operation had the following sequence of operations:

``` c
sem_post(&queue->lock);
sem_post(&queue->items);
```

Why is this better than doing these operations in the opposite order, like so?

``` c
sem_post(&queue->items);
sem_post(&queue->lock);
```

- [ ] Actually, neither order is better than the other.
- [X] The first order may lead to better performance than the second order.
- [ ] The second order can induce a deadlock while the first cannot.
- [ ] None of the above

> **Why:** `sem_post()` only increments the counter (Lec14 slides 14 and 17) and never blocks, so neither order can deadlock, which rules out choice 3. Measured on slide 21's queue (4 producers and 4 consumers moving 2M items, 16 cores, `-O2`, 7 alternating runs): releasing `lock` first averaged 1.69 s, posting `items` first 2.23 s, and the runs didn't overlap. Posting `items` while still holding `lock` wakes a consumer that then immediately blocks on `lock`. Caveat: slide 21's `queue_add` actually ends with `sem_post(&queue->items); sem_post(&queue->lock);`, the order this question calls the opposite one, so the question's "we saw in lecture" doesn't match the slide. That points toward "neither order is better" as a possible key, but the measurement shows one order is faster.

### Question 7

Say that CPU A modifies data stored at memory address `0x4060`. CPU B has a copy of the data at this address stored in its L1 cache and executes an assembly instruction that modifies this memory address, only to see that the relevant cache line has been invalidated because of a modification made by CPU A.

How did this invalidation occur?

- [X] CPU A sent a message to CPU B using a hardware bus to announce its modification of the memory address.
- [ ] The operating system saw that the address had been modified by A and marked the entry in B's L1 cache as invalid.
- [ ] After a thread unlocks a mutex, the `pthread` library has code that will tell all other CPUs about the memory address modified in the recently finished critical section.
- [ ] Trick question: No invalidation needs to occur. When CPU A modifies address `0x4060`, it tells CPU B about the address's new value.

> **Why:** Lec14 slide 30: when a core writes an address other cores have cached, "Caches exchange messages to coordinate their actions (all in Hardware)". Slide 31 shows the message: "I'm changing address 0x4061, everyone else should invalidate their copy". Neither the OS nor the `pthread` library is involved. The last choice (sending B the new value) isn't what the slides describe, and the question says B's line was invalidated.

### Question 8

Which of the following is an example of a link layer network protocol?

- [ ] IP
- [X] Ethernet
- [ ] TCP
- [ ] SMTP

> **Why:** Lec15 slide 17: "Link Layer: Exchange data between two physically connected hosts -- Ethernet, Wired (IEEE 802.3); Wi-Fi, Wireless (IEEE 802.11)". The same slide puts IP at the network layer and TCP at the transport layer. SMTP (email) belongs with HTTP and SSH at the application layer.

### Question 9

If we view the Internet as a graph, what is the purpose of the link layer?

- [ ] Routing data along a path from source node to destination node
- [ ] Sending data between two specific processes running on a source node and destination node
- [X] Sending data across an individual edge of the graph
- [ ] Interpreting bytes received from another node in the graph within the context of a specific application

> **Why:** Lec15 slide 19: "Physical and Link layers allow data to traverse one edge". Slides 20-22 match the other choices to the network layer (packets "traverse a path"), the transport layer ("sending sequence of bytes") and the application layer ("rules for interpreting bytes").

### Question 10

What is the meaning of the IPv4 address `127.0.0.1`?

- [X] This is always interpreted to mean "this computer" - data is sent and received locally.
- [ ] This is the address of `atlas.cselabs.umn.edu`
- [ ] This is the address for a DNS server
- [ ] Trick question: According to the IPv4 standards, this is not a valid IP address.

> **Why:** Lec15 slide 28: "IPv4: 127.0.0.1 always means local host, or "this computer"", and you "Can use a socket to talk to a process on same machine (often for testing)". The proj3 spec uses it the same way.
