import collections
import socket
import threading


# Raised by enqueue/dequeue options when the queue has been shut down
class ShutDownException(Exception):
    pass


# A thread-safe, bounded-size queue
class ConnectionQueue:
    # Initialize a new queue and any underlying synchronization primitives
    def __init__(self, capacity: int) -> None:
        self.elements = collections.deque()
        self.shutdown = False
        self.capacity = capacity
        # Both conditions share one lock, which guards every field above
        self.lock = threading.Lock()
        self.not_full = threading.Condition(self.lock)
        self.not_empty = threading.Condition(self.lock)

    # Add a new socket to the queue
    # This should block the calling thread if the number of elements in the
    # queue has already reached its capacity and only proceed once a slot has
    # opened up.
    # When a new item is successfully enqueued, one thread blocking on a dequeue
    # should be unblocked (if such a thread exists).
    # Raises ShutDownException if the queue was shut down while the calling thread
    # was in the middle of this operation.
    def enqueue(self, conn: socket.socket) -> None:
        with self.lock:
            # A loop, not an if: another enqueuer may take the free slot first
            while len(self.elements) >= self.capacity and not self.shutdown:
                self.not_full.wait()
            if self.shutdown:
                raise ShutDownException()
            self.elements.append(conn)
            self.not_empty.notify()

    # Remove a socket from the queue
    # This should block the calling thread if the queue is empty. The thread
    # will only proceed once it can successfully retrieve an item from the
    # queue.
    # When an item is successfully removed from the queue, one thread blocking
    # on an enqueue should be unblocked (if such a thread exists)
    # Raises ShutDownException if the queue was shut down while the calling thread
    # was in the middle of this operation.
    def dequeue(self) -> socket.socket:
        with self.lock:
            while len(self.elements) == 0 and not self.shutdown:
                self.not_empty.wait()
            # As in Part 1, dequeues keep succeeding after a shutdown until the
            # queue is empty, so no accepted connection is dropped
            if len(self.elements) == 0:
                raise ShutDownException()
            conn = self.elements.popleft()
            self.not_full.notify()
            return conn

    # Shut down the queue
    # All threads blocking on either an enqueue or dequeue are unblocked and
    # will observe a ShutDownException.
    def shut_down(self) -> None:
        with self.lock:
            self.shutdown = True
            self.not_full.notify_all()
            self.not_empty.notify_all()
