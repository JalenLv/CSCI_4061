// Unit tests for connection_queue.c, using only the interface in
// connection_queue.h. Each check prints one PASS/FAIL line. A call that should
// block is given 300 ms to (wrongly) return; a call that should wake up is
// given 2 s. Build from part1/:
//   gcc -Wall -Werror -g -pthread -I. test_cases/extra/resources/queue_test.c connection_queue.c
#include <pthread.h>
#include <stdatomic.h>
#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <unistd.h>

#include "connection_queue.h"

#define TOTAL 80000

static int failures = 0;

static void result(int ok, const char *what) {
    printf("%s: %s\n", ok ? "PASS" : "FAIL", what);
    failures += !ok;
}

static void sleep_ms(int ms) {
    struct timespec ts = {ms / 1000, (ms % 1000) * 1000000L};
    nanosleep(&ts, NULL);
}

// Waits up to ms for *flag to reach want
static int wait_for(atomic_int *flag, int want, int ms) {
    for (int waited = 0; waited < ms; waited += 10) {
        if (atomic_load(flag) >= want) {
            return 1;
        }
        sleep_ms(10);
    }
    return atomic_load(flag) >= want;
}

typedef struct {
    connection_queue_t *queue;
    int value;           // To enqueue
    int ret;             // What the call returned
    atomic_int done;     // Set once the call has returned
} call_t;

static void *do_enqueue(void *arg) {
    call_t *c = arg;
    c->ret = connection_queue_enqueue(c->queue, c->value);
    atomic_store(&c->done, 1);
    return NULL;
}

static void *do_dequeue(void *arg) {
    call_t *c = arg;
    c->ret = connection_queue_dequeue(c->queue);
    atomic_store(&c->done, 1);
    return NULL;
}

static void start(pthread_t *t, void *(*fn)(void *), call_t *c) {
    if (pthread_create(t, NULL, fn, c) != 0) {
        printf("FAIL: pthread_create\n");
        exit(1);
    }
}

// A thread still stuck after the test can't be joined; give up loudly instead
static void join_or_exit(pthread_t t, call_t *c, const char *what) {
    if (!atomic_load(&c->done)) {
        printf("FAIL: %s never returned; stopping\n", what);
        exit(1);
    }
    pthread_join(t, NULL);
}

static void fill(connection_queue_t *q, int n, int first) {
    for (int i = 0; i < n; i++) {
        if (connection_queue_enqueue(q, first + i) != 0) {
            printf("FAIL: enqueue %d of %d on a queue with room\n", i + 1, n);
            exit(1);
        }
    }
}

static void test_fifo(void) {
    connection_queue_t q;
    result(connection_queue_init(&q) == 0, "init returns 0");
    int ok = 1;
    // Several laps, so read_idx and write_idx wrap around the array
    for (int lap = 0; lap < 3; lap++) {
        fill(&q, CAPACITY, 100 * lap);
        for (int i = 0; i < CAPACITY; i++) {
            ok &= connection_queue_dequeue(&q) == 100 * lap + i;
        }
    }
    // Interleaved: never full, never empty for long
    int next_in = 0, next_out = 0;
    for (int round = 0; round < 4 * CAPACITY; round++) {
        fill(&q, 2, 1000 + next_in);
        next_in += 2;
        ok &= connection_queue_dequeue(&q) == 1000 + next_out++;
        if (next_in - next_out >= CAPACITY - 1) {
            while (next_out < next_in) {
                ok &= connection_queue_dequeue(&q) == 1000 + next_out++;
            }
        }
    }
    while (next_out < next_in) {
        ok &= connection_queue_dequeue(&q) == 1000 + next_out++;
    }
    result(ok, "elements come out in FIFO order, across wraparound");
    connection_queue_shutdown(&q);
    result(connection_queue_free(&q) == 0, "free returns 0 after shutdown");
}

static void test_full_blocks(void) {
    connection_queue_t q;
    connection_queue_init(&q);
    fill(&q, CAPACITY, 0);
    call_t c = {&q, 99, 0, 0};
    pthread_t t;
    start(&t, do_enqueue, &c);
    sleep_ms(300);
    result(!atomic_load(&c.done), "enqueue on a full queue blocks");
    int first = connection_queue_dequeue(&q);
    result(wait_for(&c.done, 1, 2000) && c.ret == 0, "a dequeue unblocks it, and it returns 0");
    join_or_exit(t, &c, "enqueue on a full queue");
    int ok = first == 0;
    for (int i = 1; i < CAPACITY; i++) {
        ok &= connection_queue_dequeue(&q) == i;
    }
    ok &= connection_queue_dequeue(&q) == 99;
    result(ok, "the blocked element is added after the others");
    connection_queue_shutdown(&q);
    connection_queue_free(&q);
}

static void test_empty_blocks(void) {
    connection_queue_t q;
    connection_queue_init(&q);
    call_t c = {&q, 0, 0, 0};
    pthread_t t;
    start(&t, do_dequeue, &c);
    sleep_ms(300);
    result(!atomic_load(&c.done), "dequeue on an empty queue blocks");
    connection_queue_enqueue(&q, 42);
    result(wait_for(&c.done, 1, 2000) && c.ret == 42,
           "an enqueue unblocks it, and it returns that element");
    join_or_exit(t, &c, "dequeue on an empty queue");
    connection_queue_shutdown(&q);
    connection_queue_free(&q);
}

static void test_shutdown_wakes_dequeuers(void) {
    connection_queue_t q;
    connection_queue_init(&q);
    call_t c[3] = {{&q, 0, 0, 0}, {&q, 0, 0, 0}, {&q, 0, 0, 0}};
    pthread_t t[3];
    for (int i = 0; i < 3; i++) {
        start(&t[i], do_dequeue, &c[i]);
    }
    sleep_ms(300);
    connection_queue_shutdown(&q);
    int ok = 1;
    for (int i = 0; i < 3; i++) {
        ok &= wait_for(&c[i].done, 1, 2000) && c[i].ret == -1;
    }
    result(ok, "shutdown wakes all 3 blocked dequeuers, each gets -1");
    for (int i = 0; i < 3; i++) {
        join_or_exit(t[i], &c[i], "dequeue blocked at shutdown");
    }
    connection_queue_free(&q);
}

static void test_shutdown_wakes_enqueuers(void) {
    connection_queue_t q;
    connection_queue_init(&q);
    fill(&q, CAPACITY, 0);
    call_t c[2] = {{&q, 98, 0, 0}, {&q, 99, 0, 0}};
    pthread_t t[2];
    for (int i = 0; i < 2; i++) {
        start(&t[i], do_enqueue, &c[i]);
    }
    sleep_ms(300);
    connection_queue_shutdown(&q);
    int ok = 1;
    for (int i = 0; i < 2; i++) {
        ok &= wait_for(&c[i].done, 1, 2000) && c[i].ret == -1;
    }
    result(ok, "shutdown wakes both blocked enqueuers, each gets -1");
    for (int i = 0; i < 2; i++) {
        join_or_exit(t[i], &c[i], "enqueue blocked at shutdown");
    }

    // The spec: dequeues continue until the queue is empty, then fail
    ok = 1;
    for (int i = 0; i < CAPACITY; i++) {
        ok &= connection_queue_dequeue(&q) == i;
    }
    result(ok, "after shutdown, dequeue still returns the queued elements in order");
    result(connection_queue_dequeue(&q) == -1, "then dequeue returns -1 without blocking");
    result(connection_queue_enqueue(&q, 7) == -1, "enqueue after shutdown returns -1");
    result(connection_queue_dequeue(&q) == -1, "and adds nothing");
    connection_queue_free(&q);
}

static connection_queue_t stress_q;
static atomic_int received;
static atomic_int seen[TOTAL];
static atomic_int shut_down;
static int per_producer;

static void *producer(void *arg) {
    int base = (int) (long) arg * per_producer;
    for (int i = 0; i < per_producer; i++) {
        if (connection_queue_enqueue(&stress_q, base + i) != 0) {
            printf("FAIL: enqueue returned an error during the stress test\n");
            exit(1);
        }
    }
    return NULL;
}

// A broken queue can return an element twice, one never enqueued, or -1
// before any shutdown (a worker would quit, shrinking the pool). Stop at once
// rather than let it loop on a corrupted length.
static void *consumer(void *arg) {
    int v;
    while ((v = connection_queue_dequeue(&stress_q)) != -1) {
        if (v < 0 || v >= TOTAL) {
            printf("FAIL: dequeued a value that was never enqueued; stopping\n");
            exit(1);
        }
        if (atomic_fetch_add(&seen[v], 1) != 0) {
            printf("FAIL: an element was dequeued twice; stopping\n");
            exit(1);
        }
        atomic_fetch_add(&received, 1);
    }
    if (!atomic_load(&shut_down)) {
        printf("FAIL: dequeue returned -1 before the queue was shut down; stopping\n");
        exit(1);
    }
    return NULL;
}

// Producers fill the queue while consumers race for elements. With one
// producer and many consumers the queue is mostly empty, so most consumers
// wait, and a woken consumer often finds its element already taken by one
// that never had to wait: the case a while loop around the wait handles
// (with an if, it finds the queue empty and fails or returns garbage).
static void stress(int n_producers, int n_consumers, const char *what) {
    per_producer = TOTAL / n_producers;
    atomic_store(&received, 0);
    atomic_store(&shut_down, 0);
    for (int i = 0; i < TOTAL; i++) {
        atomic_store(&seen[i], 0);
    }
    connection_queue_init(&stress_q);
    pthread_t p[8], c[8];
    for (long i = 0; i < n_consumers; i++) {
        pthread_create(&c[i], NULL, consumer, NULL);
    }
    for (long i = 0; i < n_producers; i++) {
        pthread_create(&p[i], NULL, producer, (void *) i);
    }
    for (int i = 0; i < n_producers; i++) {
        pthread_join(p[i], NULL);
    }
    // Shut down only once every element has been taken, so the check doesn't
    // depend on what shutdown does with leftovers
    if (!wait_for(&received, TOTAL, 20000)) {
        printf("FAIL: consumers stalled at %d of %d elements\n", atomic_load(&received), TOTAL);
        exit(1);
    }
    atomic_store(&shut_down, 1);
    connection_queue_shutdown(&stress_q);
    for (int i = 0; i < n_consumers; i++) {
        pthread_join(c[i], NULL);
    }
    result(1, what);
    connection_queue_free(&stress_q);
}

int main(void) {
    setvbuf(stdout, NULL, _IONBF, 0);
    test_fifo();
    test_full_blocks();
    test_empty_blocks();
    test_shutdown_wakes_dequeuers();
    test_shutdown_wakes_enqueuers();
    stress(4, 4, "4 producers, 4 consumers: each of 80000 elements dequeued exactly once");
    stress(1, 8, "1 producer, 8 consumers, mostly waiting: each element dequeued exactly once");
    printf("%d failed\n", failures);
    return failures != 0;
}
