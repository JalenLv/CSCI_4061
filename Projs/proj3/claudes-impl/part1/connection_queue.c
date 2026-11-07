#include "connection_queue.h"

#include <stdio.h>
#include <string.h>

// pthread functions return an error number instead of setting errno
static void report(const char *func, int err) { fprintf(stderr, "%s: %s\n", func, strerror(err)); }

int connection_queue_init(connection_queue_t *queue) {
    queue->length = 0;
    queue->read_idx = 0;
    queue->write_idx = 0;
    queue->shutdown = 0;

    int result;
    if ((result = pthread_mutex_init(&queue->lock, NULL)) != 0) {
        report("pthread_mutex_init", result);
        return -1;
    }
    if ((result = pthread_cond_init(&queue->not_full, NULL)) != 0) {
        report("pthread_cond_init", result);
        pthread_mutex_destroy(&queue->lock);
        return -1;
    }
    if ((result = pthread_cond_init(&queue->not_empty, NULL)) != 0) {
        report("pthread_cond_init", result);
        pthread_cond_destroy(&queue->not_full);
        pthread_mutex_destroy(&queue->lock);
        return -1;
    }
    return 0;
}

int connection_queue_enqueue(connection_queue_t *queue, int connection_fd) {
    int result;
    if ((result = pthread_mutex_lock(&queue->lock)) != 0) {
        report("pthread_mutex_lock", result);
        return -1;
    }

    // A loop, not an if: wakeups can be spurious, and another enqueuer may
    // take the free slot between the signal and this thread reacquiring the lock
    while (queue->length == CAPACITY && !queue->shutdown) {
        if ((result = pthread_cond_wait(&queue->not_full, &queue->lock)) != 0) {
            report("pthread_cond_wait", result);
            pthread_mutex_unlock(&queue->lock);
            return -1;
        }
    }
    if (queue->shutdown) {
        if ((result = pthread_mutex_unlock(&queue->lock)) != 0) {
            report("pthread_mutex_unlock", result);
        }
        return -1;
    }

    queue->client_fds[queue->write_idx] = connection_fd;
    queue->write_idx = (queue->write_idx + 1) % CAPACITY;
    queue->length++;

    // Signal on every enqueue, not only when the queue was empty: two
    // enqueues can land before the first woken dequeuer runs
    if ((result = pthread_cond_signal(&queue->not_empty)) != 0) {
        report("pthread_cond_signal", result);
        pthread_mutex_unlock(&queue->lock);
        return -1;
    }
    if ((result = pthread_mutex_unlock(&queue->lock)) != 0) {
        report("pthread_mutex_unlock", result);
        return -1;
    }
    return 0;
}

int connection_queue_dequeue(connection_queue_t *queue) {
    int result;
    if ((result = pthread_mutex_lock(&queue->lock)) != 0) {
        report("pthread_mutex_lock", result);
        return -1;
    }

    while (queue->length == 0 && !queue->shutdown) {
        if ((result = pthread_cond_wait(&queue->not_empty, &queue->lock)) != 0) {
            report("pthread_cond_wait", result);
            pthread_mutex_unlock(&queue->lock);
            return -1;
        }
    }
    // After a shutdown, dequeues keep succeeding until the queue is empty
    if (queue->length == 0) {
        if ((result = pthread_mutex_unlock(&queue->lock)) != 0) {
            report("pthread_mutex_unlock", result);
        }
        return -1;
    }

    int fd = queue->client_fds[queue->read_idx];
    queue->read_idx = (queue->read_idx + 1) % CAPACITY;
    queue->length--;

    if ((result = pthread_cond_signal(&queue->not_full)) != 0) {
        report("pthread_cond_signal", result);
        pthread_mutex_unlock(&queue->lock);
        return -1;
    }
    if ((result = pthread_mutex_unlock(&queue->lock)) != 0) {
        report("pthread_mutex_unlock", result);
        return -1;
    }
    return fd;
}

int connection_queue_shutdown(connection_queue_t *queue) {
    int result;
    if ((result = pthread_mutex_lock(&queue->lock)) != 0) {
        report("pthread_mutex_lock", result);
        return -1;
    }
    queue->shutdown = 1;
    int ret_val = 0;
    if ((result = pthread_cond_broadcast(&queue->not_full)) != 0) {
        report("pthread_cond_broadcast", result);
        ret_val = -1;
    }
    if ((result = pthread_cond_broadcast(&queue->not_empty)) != 0) {
        report("pthread_cond_broadcast", result);
        ret_val = -1;
    }
    if ((result = pthread_mutex_unlock(&queue->lock)) != 0) {
        report("pthread_mutex_unlock", result);
        ret_val = -1;
    }
    return ret_val;
}

int connection_queue_free(connection_queue_t *queue) {
    int result;
    int ret_val = 0;
    if ((result = pthread_mutex_destroy(&queue->lock)) != 0) {
        report("pthread_mutex_destroy", result);
        ret_val = -1;
    }
    if ((result = pthread_cond_destroy(&queue->not_full)) != 0) {
        report("pthread_cond_destroy", result);
        ret_val = -1;
    }
    if ((result = pthread_cond_destroy(&queue->not_empty)) != 0) {
        report("pthread_cond_destroy", result);
        ret_val = -1;
    }
    return ret_val;
}
