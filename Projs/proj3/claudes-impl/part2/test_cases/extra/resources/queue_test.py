#!/usr/bin/env python3
"""Unit tests for connection_queue.py, run from part2/. Each check prints one
PASS/FAIL line. A call that should block is given 300 ms to (wrongly) return;
a call that should wake up is given 2 s. Plain ints stand in for sockets."""
import os
import sys
import threading
import time

sys.path.insert(0, os.getcwd())
from connection_queue import ConnectionQueue, ShutDownException  # noqa: E402

CAPACITY = 5
failures = 0


def result(ok, what):
    global failures
    print(f"{'PASS' if ok else 'FAIL'}: {what}", flush=True)
    failures += not ok


class Call:
    """Runs one queue operation in its own thread and records the outcome."""

    def __init__(self, fn, *args):
        self.done = threading.Event()
        self.value = self.raised = None
        self.thread = threading.Thread(target=self._run, args=(fn, args), daemon=True)
        self.thread.start()

    def _run(self, fn, args):
        try:
            self.value = fn(*args)
        except ShutDownException:
            self.raised = "ShutDownException"
        except Exception as e:  # Anything else is a bug in the queue
            self.raised = repr(e)
        self.done.set()


def fill(q, n, first=0):
    for i in range(n):
        q.enqueue(first + i)


def test_fifo():
    q = ConnectionQueue(CAPACITY)
    ok = True
    for lap in range(3):
        fill(q, CAPACITY, 100 * lap)
        ok &= [q.dequeue() for _ in range(CAPACITY)] == list(range(100 * lap, 100 * lap + CAPACITY))
    result(ok, "elements come out in FIFO order")


def test_full_blocks():
    q = ConnectionQueue(CAPACITY)
    fill(q, CAPACITY)
    c = Call(q.enqueue, 99)
    result(not c.done.wait(0.3), "enqueue on a full queue blocks")
    first = q.dequeue()
    result(c.done.wait(2) and c.raised is None, "a dequeue unblocks it")
    rest = [q.dequeue() for _ in range(CAPACITY)]
    result([first] + rest == list(range(CAPACITY)) + [99], "the blocked element is added after the others")


def test_empty_blocks():
    q = ConnectionQueue(CAPACITY)
    c = Call(q.dequeue)
    result(not c.done.wait(0.3), "dequeue on an empty queue blocks")
    q.enqueue(42)
    result(c.done.wait(2) and c.value == 42, "an enqueue unblocks it, and it returns that element")


def test_shutdown_wakes_dequeuers():
    q = ConnectionQueue(CAPACITY)
    calls = [Call(q.dequeue) for _ in range(3)]
    time.sleep(0.3)
    q.shut_down()
    result(all(c.done.wait(2) and c.raised == "ShutDownException" for c in calls),
           "shut_down wakes all 3 blocked dequeuers, each raises ShutDownException")


def test_shutdown_wakes_enqueuers():
    q = ConnectionQueue(CAPACITY)
    fill(q, CAPACITY)
    calls = [Call(q.enqueue, 98), Call(q.enqueue, 99)]
    time.sleep(0.3)
    q.shut_down()
    result(all(c.done.wait(2) and c.raised == "ShutDownException" for c in calls),
           "shut_down wakes both blocked enqueuers, each raises ShutDownException")
    # As in Part 1: dequeues continue until the queue is empty, then fail
    c = Call(lambda: [q.dequeue() for _ in range(CAPACITY)])
    result(c.done.wait(2) and c.value == list(range(CAPACITY)),
           "after shut_down, dequeue still returns the queued elements in order")
    c = Call(q.dequeue)
    result(c.done.wait(2) and c.raised == "ShutDownException", "then dequeue raises ShutDownException")
    c = Call(q.enqueue, 7)
    result(c.done.wait(2) and c.raised == "ShutDownException", "enqueue after shut_down raises ShutDownException")


def stress(n_producers, n_consumers, total, what):
    # With one producer and many consumers the queue is mostly empty, so most
    # consumers wait, and a woken consumer often finds its element already
    # taken by one that never had to wait: the case a while loop handles
    q = ConnectionQueue(CAPACITY)
    per = total // n_producers
    seen, errors, lock = [], [], threading.Lock()
    shut = threading.Event()

    def produce(base):
        for i in range(per):
            q.enqueue(base + i)

    def consume():
        while True:
            try:
                v = q.dequeue()
            except ShutDownException:
                if not shut.is_set():  # A worker would quit, shrinking the pool
                    with lock:
                        errors.append("ShutDownException before shut_down()")
                return
            except Exception as e:  # A broken queue, e.g. popleft() on an empty deque
                with lock:
                    errors.append(repr(e))
                return
            with lock:
                seen.append(v)

    consumers = [threading.Thread(target=consume, daemon=True) for _ in range(n_consumers)]
    producers = [threading.Thread(target=produce, args=(i * per,), daemon=True) for i in range(n_producers)]
    for t in consumers + producers:
        t.start()
    for t in producers:
        t.join(20)
    deadline = time.monotonic() + 20
    while len(seen) < total and not errors and time.monotonic() < deadline:
        time.sleep(0.01)
    # Shut down only once everything was taken, so the check doesn't depend
    # on what shut_down does with leftovers
    shut.set()
    q.shut_down()
    for t in consumers:
        t.join(2)
    result(sorted(seen) == list(range(total)) and not errors and not any(t.is_alive() for t in consumers),
           what)


test_fifo()
test_full_blocks()
test_empty_blocks()
test_shutdown_wakes_dequeuers()
test_shutdown_wakes_enqueuers()
stress(4, 4, 20000, "4 producers, 4 consumers: each of 20000 elements dequeued exactly once")
stress(1, 8, 20000, "1 producer, 8 consumers, mostly waiting: each element dequeued exactly once")
print(f"{failures} failed", flush=True)
os._exit(failures != 0)  # Don't wait for threads a broken queue left blocked
