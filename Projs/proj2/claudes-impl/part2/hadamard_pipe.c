#include <Python.h>
#include <assert.h>
#include <errno.h>
#include <limits.h>
#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <unistd.h>

#include "hadamard.h"

/*
 * Represents one matrix element communicated over a pipe
 * 'idx': Element index (in flattened representation)
 * 'value': Element value
 */
struct element {
    unsigned idx;
    int value;
};

/*
 * Number of elements a worker sends with one write(). Writes of at most PIPE_BUF bytes
 * are atomic, so the records of different workers can never interleave mid-record.
 * 4096 / 8 = 512 elements.
 */
#define ELEMS_PER_WRITE (PIPE_BUF / sizeof(struct element))

/*
 * Function executed in each child process to compute its portion of the hadamard product
 * Declared here so it can be referenced in hadamard_pipe(). Complete the
 * definition at the bottom of this file.
 *
 * 'a': Pointer to elements for first matrix operand
 * 'b': Pointer to elements for second matrix operand
 * 'start': Index (in flattened representation) of first element of matrix portion
 * 'end': Index (in flattened representation) one past the last element of matrix portion
 * 'pipe_fd': Descriptor of write end of pipe to use to send results back to coordinator
 *
 * Returns 0 on success or -1 on failure
 */
static int worker(const int *a, const int *b, unsigned start, unsigned end, int pipe_fd);

/*
 * Computes the portion of an 'n_elems'-element matrix that worker number 'worker_idx'
 * (counting from 0) out of 'n_workers' is responsible for, as the half-open index range
 * [*start, *end). Every worker gets ceil(n_elems / n_workers) elements except at the
 * end of the matrix, where the last workers get fewer, or none at all (*start == *end)
 * when there are more workers than elements.
 */
static void get_chunk(unsigned worker_idx, unsigned n_workers, unsigned n_elems, unsigned *start,
                      unsigned *end) {
    // Integer ceiling of n_elems / n_workers, written so it cannot overflow
    unsigned chunk_size = n_elems / n_workers + (n_elems % n_workers != 0);
    *start = min(worker_idx * chunk_size, n_elems);
    *end = min(*start + chunk_size, n_elems);
}

/*
 * Waits for each of the 'n_pids' child processes in 'pids'.
 * Raises a Python exception if a wait fails or a child did not exit with status 0,
 * unless an exception is already set (the first error is the one reported).
 * Every child is waited for either way, so none is left behind as a zombie.
 */
static void wait_for_children(const pid_t *pids, unsigned n_pids) {
    for (unsigned i = 0; i < n_pids; i++) {
        int status;
        pid_t ret;
        do {
            ret = waitpid(pids[i], &status, 0);
        } while (ret == -1 && errno == EINTR);

        if (PyErr_Occurred()) {
            continue;
        }
        if (ret == -1) {
            PyErr_SetFromErrno(PyExc_OSError);
        } else if (WIFSIGNALED(status)) {
            PyErr_Format(PyExc_RuntimeError, "Worker process %d killed by signal %d", (int) pids[i],
                         WTERMSIG(status));
        } else if (!WIFEXITED(status) || WEXITSTATUS(status) != 0) {
            PyErr_Format(PyExc_RuntimeError, "Worker process %d failed", (int) pids[i]);
        }
    }
}

/*
 * Reads 'struct element' records from 'read_fd' until every write end of the pipe has
 * been closed, storing each value at its index in 'c' ('n_elems' elements long).
 * Nothing guarantees that one read() returns whole records, so a partial record is
 * kept at the front of the buffer until the rest of it arrives.
 * Raises a Python exception on failure.
 */
static void collect_results(int read_fd, int *c, unsigned n_elems) {
    struct element buf[ELEMS_PER_WRITE];
    size_t n_bytes = 0;    // Bytes currently held in 'buf'
    while (1) {
        ssize_t n_read = read(read_fd, (char *) buf + n_bytes, sizeof(buf) - n_bytes);
        if (n_read == -1) {
            if (errno == EINTR) {
                continue;
            }
            PyErr_SetFromErrno(PyExc_OSError);
            return;
        }
        if (n_read == 0) {
            break;    // End of file: no process has the write end open any more
        }
        n_bytes += n_read;

        size_t n_records = n_bytes / sizeof(struct element);
        for (size_t i = 0; i < n_records; i++) {
            // Never trust an index read from outside this process to stay in bounds
            if (buf[i].idx >= n_elems) {
                PyErr_Format(PyExc_RuntimeError, "Received out-of-range element index %u",
                             buf[i].idx);
                return;
            }
            c[buf[i].idx] = buf[i].value;
        }
        n_bytes -= n_records * sizeof(struct element);
        memmove(buf, buf + n_records, n_bytes);
    }

    if (n_bytes != 0) {
        PyErr_SetString(PyExc_RuntimeError, "Pipe closed in the middle of a matrix element");
    }
}

// The provided read_matrix_from_file() sets a Python error itself in the original starter
// but only prints to stderr in the 10-22 update. Raise one if it didn't, without clobbering
// one it did; the message is the updated starter's own.
static void read_failed(void) {
    if (!PyErr_Occurred()) {
        PyErr_SetString(PyExc_OSError, "Failed to read matrix data from file");
    }
}

PyObject *hadamard_pipe(PyObject *self, PyObject *args) {
    (void) self;    // The module object, passed to every module-level function; not needed
    const char *mat_a_file;
    const char *mat_b_file;
    int n_workers;
    // "i", not the starter's "I": "I" does no overflow checking, so -1 would arrive as
    // 4294967295 workers. "i" raises OverflowError for values that don't fit in an int,
    // and the check below rejects 0 and negative counts.
    if (!PyArg_ParseTuple(args, "ssi", &mat_a_file, &mat_b_file, &n_workers)) {
        // ParseTuple sets up a Python exception on our behalf
        return NULL;
    }
    if (n_workers <= 0) {
        PyErr_SetString(PyExc_ValueError, "Number of workers must be at least 1");
        return NULL;
    }
    unsigned n_procs = (unsigned) n_workers;

    unsigned a_rows;
    unsigned a_cols;
    int *mat_a_data = read_matrix_from_file(mat_a_file, &a_rows, &a_cols, 0);
    if (mat_a_data == NULL) {
        read_failed();
        return NULL;
    }
    unsigned b_rows;
    unsigned b_cols;
    int *mat_b_data = read_matrix_from_file(mat_b_file, &b_rows, &b_cols, 0);
    if (mat_b_data == NULL) {
        read_failed();
        free(mat_a_data);
        return NULL;
    }
    // Not an assert(): Python builds extensions with -DNDEBUG, which compiles asserts out
    if (a_rows != b_rows || a_cols != b_cols) {
        PyErr_SetString(PyExc_ValueError, "Matrix dimensions do not match");
        free(mat_a_data);
        free(mat_b_data);
        return NULL;
    }
    unsigned n_elems = a_rows * a_cols;

    pid_t *pids = malloc(n_procs * sizeof(pid_t));
    if (pids == NULL) {
        free(mat_a_data);
        free(mat_b_data);
        return PyErr_NoMemory();
    }

    // One pipe shared by all children: they all write into it, only the parent reads
    int pipe_fds[2];
    if (pipe(pipe_fds) == -1) {
        PyErr_SetFromErrno(PyExc_OSError);
        free(pids);
        free(mat_a_data);
        free(mat_b_data);
        return NULL;
    }

    unsigned n_forked = 0;
    // Anything still in stdout's buffer would be copied into every child, and each
    // child's exit() would print it again. Python writes through its own buffer, but
    // an earlier call into this module may have left output in C's stdout buffer.
    if (fflush(stdout) == EOF) {
        PyErr_SetFromErrno(PyExc_OSError);
    }
    while (!PyErr_Occurred() && n_forked < n_procs) {
        pid_t pid = fork();
        if (pid == -1) {
            PyErr_SetFromErrno(PyExc_OSError);
            break;    // Still wait for the children that were forked
        }
        if (pid == 0) {
            // Child: it only writes, so it closes the read end. If it kept the read end
            // open, a parent that stopped reading could leave it blocked in write().
            int status = 0;
            if (close(pipe_fds[0]) == -1) {
                perror("close");
                status = 1;
            } else {
                unsigned start;
                unsigned end;
                get_chunk(n_forked, n_procs, n_elems, &start, &end);
                if (worker(mat_a_data, mat_b_data, start, end, pipe_fds[1]) == -1) {
                    status = 1;
                }
            }
            // fork() gave the child its own (copy-on-write) copies of these heap blocks,
            // so it frees them too. exit() keeps the child from returning into Python.
            free(pids);
            free(mat_a_data);
            free(mat_b_data);
            if (close(pipe_fds[1]) == -1) {
                perror("close");
                status = 1;
            }
            exit(status);
        }
        pids[n_forked] = pid;
        n_forked++;
    }

    // The parent never writes. Its copy of the write end must be closed, or read() would
    // never return 0: end of file only comes once every write end is closed.
    if (close(pipe_fds[1]) == -1 && !PyErr_Occurred()) {
        PyErr_SetFromErrno(PyExc_OSError);
    }

    int *mat_c_data = NULL;
    if (!PyErr_Occurred()) {
        mat_c_data = malloc(n_elems * sizeof(int));
        if (mat_c_data == NULL) {
            PyErr_NoMemory();
        } else {
            collect_results(pipe_fds[0], mat_c_data, n_elems);
        }
    }

    // On an error path some children may still be writing. With the read end closed
    // their write() fails (EPIPE) instead of blocking forever, so the waits below finish.
    if (close(pipe_fds[0]) == -1 && !PyErr_Occurred()) {
        PyErr_SetFromErrno(PyExc_OSError);
    }
    wait_for_children(pids, n_forked);

    if (!PyErr_Occurred()) {
        print_matrix(mat_c_data, a_rows, a_cols);
    }
    free(mat_c_data);
    free(pids);
    free(mat_a_data);
    free(mat_b_data);
    if (PyErr_Occurred()) {
        return NULL;
    }
    Py_RETURN_NONE;
}

/*
 * Writes all 'len' bytes at 'buf' to 'fd', retrying after EINTR or a short write.
 * (A blocking pipe never splits a write of at most PIPE_BUF bytes, so the retry for a
 * short write is only there to make this correct for any 'fd'.)
 * Returns 0 on success or -1 on failure, after printing an error message.
 */
static int write_all(int fd, const void *buf, size_t len) {
    const char *pos = buf;
    while (len > 0) {
        ssize_t n_written = write(fd, pos, len);
        if (n_written == -1) {
            if (errno == EINTR) {
                continue;
            }
            perror("write");
            return -1;
        }
        pos += n_written;
        len -= n_written;
    }
    return 0;
}

// Called once by each child process
static int worker(const int *a, const int *b, unsigned start, unsigned end, int pipe_fd) {
    // A and B were read into memory before fork(), so there are no files to open here.
    // Each result goes out as a 'struct element' so the parent knows where it belongs.
    struct element batch[ELEMS_PER_WRITE];
    size_t n_batched = 0;
    for (unsigned i = start; i < end; i++) {
        batch[n_batched].idx = i;
        batch[n_batched].value = a[i] * b[i];
        n_batched++;
        if (n_batched == ELEMS_PER_WRITE || i + 1 == end) {
            if (write_all(pipe_fd, batch, n_batched * sizeof(struct element)) == -1) {
                return -1;
            }
            n_batched = 0;
        }
    }
    return 0;
}
