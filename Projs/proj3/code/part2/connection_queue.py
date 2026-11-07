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
        # TODO Add the necessary thread synchronization primitives

    # Add a new socket to the queue
    # This should block the calling thread if the number of elements in the
    # queue has already reached its capacity and only proceed once a slot has
    # opened up.
    # When a new item is successfully enqueued, one thread blocking on a dequeue
    # should be unblocked (if such a thread exists).
    # Raises ShutDownException if the queue was shut down while the calling thread
    # was in the middle of this operation.
    def enqueue(self, conn: socket.socket) -> None:
        raise NotImplementedError("TODO: ConnectionQueue enqueue")

    # Remove a socket from the queue
    # This should block the calling thread if the queue is empty. The thread
    # will only proceed once it can successfully retrieve an item from the
    # queue.
    # When an item is successfully removed from the queue, one thread blocking
    # on an enqueue should be unblocked (if such a thread exists)
    # Raises ShutDownException if the queue was shut down while the calling thread
    # was in the middle of this operation.
    def dequeue(self) -> socket.socket:
        raise NotImplementedError("TODO: ConnectionQueue dequeue")

    # Shut down the queue
    # All threads blocking on either an enqueue or dequeue are unblocked and
    # will observe a ShutDownException.
    def shut_down(self) -> None:
        raise NotImplementedError("TODO: ConnectionQueue shut_down")
