import random
import threading
import time

RAND_SEED = 4061
N_THREADS = 10
MAX_SLEEP_SEC = 3


# Implements a thread barrier
class Barrier:
    def __init__(self, total_threads: int) -> None:
        self.total_threads = total_threads
        self.waiting_threads = 0
        # TODO Add the necessary synchronization primitives
        self.mutex = threading.Semaphore(1)
        self.barrier = threading.Semaphore(0)

    # When a thread calls 'wait', it will block until all other threads have
    # also reached and started a call to 'wait'. At that point, all threads
    # are allowed to proceed
    def wait(self) -> None:
        self.mutex.acquire()
        self.waiting_threads += 1
        self.mutex.release()

        if self.waiting_threads == self.total_threads:
            self.barrier.release() # unblock ONE thread

        self.barrier.acquire()
        # once we are unblocked, it's our duty to unblock the next thread
        self.barrier.release()


def thread_func(index: int, barrier: Barrier) -> None:
    # Each thread sleeps for a random initial delay
    time.sleep(random.random() * MAX_SLEEP_SEC)
    print(f"Thread {index} about to enter barrier")

    barrier.wait()

    print(f"Thread {index} past barrier")


if __name__ == "__main__":
    # Set up random seed and barrier
    random.seed(RAND_SEED)
    barrier = Barrier(N_THREADS)

    # Start all threads
    threads = []
    for i in range(N_THREADS):
        t = threading.Thread(target=thread_func, args=(i, barrier))
        threads.append(t)
        t.start()

    # Wait for all threads to complete
    for t in threads:
        t.join()
