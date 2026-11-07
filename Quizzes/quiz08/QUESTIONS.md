# CSCI 4061 -- Quiz 08

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
- [ ] Circular Wait

### Question 2

Say we have a binary tree data structure shared among multiple threads and use a `pthread_rwlock_t` to synchronize access to the tree. Which of the following operations would require a call to `pthread_rwlock_wrlock()`?

- [ ] Adding an Element to the Tree
- [ ] Deleting an Element from the Tree
- [ ] Rearranging the Tree so that it is Balanced
- [ ] All of the Above

### Question 3

For the binary tree scenario described above, which of the following operations would require a call to `pthread_rwlock_rdlock()`?

- [ ] Searching the Tree
- [ ] Adding an element to the Tree
- [ ] Deleting an Element from the Tree
- [ ] Rearranging the Tree so that it is Balanced

### Question 4

If we consider the performance of all threads in aggregate, which operation will benefit the most from the use of a `pthread_rwlock_t` instead of a `pthread_mutex_t` to synchronize access to the binary tree?

- [ ] Searching the Tree
- [ ] Adding an element to the Tree
- [ ] Deleting an Element from the Tree
- [ ] Rearranging the Tree so that it is Balanced

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
- [ ] The second order can induce a deadlock while the first cannot.
- [ ] None of the above

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
- [ ] The first order may lead to better performance than the second order.
- [ ] The second order can induce a deadlock while the first cannot.
- [ ] None of the above

### Question 7

Say that CPU A modifies data stored at memory address `0x4060`. CPU B has a copy of the data at this address stored in its L1 cache and executes an assembly instruction that modifies this memory address, only to see that the relevant cache line has been invalidated because of a modification made by CPU A.

How did this invalidation occur?

- [ ] CPU A sent a message to CPU B using a hardware bus to announce its modification of the memory address.
- [ ] The operating system saw that the address had been modified by A and marked the entry in B's L1 cache as invalid.
- [ ] After a thread unlocks a mutex, the `pthread` library has code that will tell all other CPUs about the memory address modified in the recently finished critical section.
- [ ] Trick question: No invalidation needs to occur. When CPU A modifies address `0x4060`, it tells CPU B about the address's new value.

### Question 8

Which of the following is an example of a link layer network protocol?

- [ ] IP
- [ ] Ethernet
- [ ] TCP
- [ ] SMTP

### Question 9

If we view the Internet as a graph, what is the purpose of the link layer?

- [ ] Routing data along a path from source node to destination node
- [ ] Sending data between two specific processes running on a source node and destination node
- [ ] Sending data across an individual edge of the graph
- [ ] Interpreting bytes received from another node in the graph within the context of a specific application

### Question 10

What is the meaning of the IPv4 address `127.0.0.1`?

- [ ] This is always interpreted to mean "this computer" - data is sent and received locally.
- [ ] This is the address of `atlas.cselabs.umn.edu`
- [ ] This is the address for a DNS server
- [ ] Trick question: According to the IPv4 standards, this is not a valid IP address.
