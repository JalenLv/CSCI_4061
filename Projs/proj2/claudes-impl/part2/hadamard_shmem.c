#define _GNU_SOURCE

#include <Python.h>
#include <assert.h>
#include <errno.h>
#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/mman.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <unistd.h>

#include "hadamard.h"

/*
 * Function executed in each child process to compute its portion of the hadamard product
 * Declared here so it can be referenced in hadamard_shmem. Fill in the definition at
 * the end of this file.
 *
 * 'a': Pointer to shared memory segment storing first matrix operand
 * 'b': Pointer to shared memory segment storing second matrix operand
 * 'c': Pointer to shared memory segment storing result matrix
 * 'start': Index (in flattened representation) of first element of matrix portion
 * 'end': Index (in flattened representation) one past the last element of matrix portion
 * 'mapped_size': Size (in bytes) of each shared memory segment
 *
 * Returns 0 on success or -1 on failure
 */
static int worker(int *a, int *b, int *c, unsigned start, unsigned end, unsigned mapped_size);

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
 * Unmaps a shared memory segment. Raises a Python exception if that fails, unless
 * an exception is already set.
 */
static void unmap_segment(int *segment, unsigned size) {
    if (munmap(segment, size) == -1 && !PyErr_Occurred()) {
        PyErr_SetFromErrno(PyExc_OSError);
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

PyObject *hadamard_shmem(PyObject *self, PyObject *args) {
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

    // With sh_mem = 1, A and B are placed in MAP_SHARED segments that children inherit
    unsigned a_rows;
    unsigned a_cols;
    int *mat_a_data = read_matrix_from_file(mat_a_file, &a_rows, &a_cols, 1);
    if (mat_a_data == NULL) {
        read_failed();
        return NULL;
    }
    unsigned a_size = a_rows * a_cols * sizeof(int);
    unsigned b_rows;
    unsigned b_cols;
    int *mat_b_data = read_matrix_from_file(mat_b_file, &b_rows, &b_cols, 1);
    if (mat_b_data == NULL) {
        read_failed();
        unmap_segment(mat_a_data, a_size);
        return NULL;
    }
    // Not an assert(): Python builds extensions with -DNDEBUG, which compiles asserts out
    if (a_rows != b_rows || a_cols != b_cols) {
        PyErr_SetString(PyExc_ValueError, "Matrix dimensions do not match");
        unmap_segment(mat_a_data, a_size);
        unmap_segment(mat_b_data, b_cols * b_rows * sizeof(int));
        return NULL;
    }
    unsigned n_elems = a_rows * a_cols;
    unsigned mapped_size = a_size;

    // Set up C the same way read_matrix_from_file() sets up A and B. MAP_SHARED is what
    // makes the children's writes visible to the parent; with MAP_PRIVATE each child
    // would write to its own copy-on-write copy and the parent would see none of it.
    int *mat_c_data =
        mmap(NULL, mapped_size, PROT_READ | PROT_WRITE, MAP_SHARED | MAP_ANONYMOUS, -1, 0);
    if (mat_c_data == MAP_FAILED) {
        PyErr_SetFromErrno(PyExc_OSError);
        unmap_segment(mat_a_data, mapped_size);
        unmap_segment(mat_b_data, mapped_size);
        return NULL;
    }

    pid_t *pids = malloc(n_procs * sizeof(pid_t));
    if (pids == NULL) {
        PyErr_NoMemory();
        unmap_segment(mat_c_data, mapped_size);
        unmap_segment(mat_a_data, mapped_size);
        unmap_segment(mat_b_data, mapped_size);
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
            // Child: the heap copy of 'pids' is its own, so it frees it too. The worker
            // unmaps the three segments. exit() keeps it from returning into Python.
            free(pids);
            unsigned start;
            unsigned end;
            get_chunk(n_forked, n_procs, n_elems, &start, &end);
            int ret = worker(mat_a_data, mat_b_data, mat_c_data, start, end, mapped_size);
            exit(ret == 0 ? 0 : 1);
        }
        pids[n_forked] = pid;
        n_forked++;
    }

    // Children write disjoint parts of C, so the only coordination needed is to
    // wait for all of them before reading it
    wait_for_children(pids, n_forked);

    if (!PyErr_Occurred()) {
        print_matrix(mat_c_data, a_rows, a_cols);
    }
    free(pids);
    unmap_segment(mat_c_data, mapped_size);
    unmap_segment(mat_a_data, mapped_size);
    unmap_segment(mat_b_data, mapped_size);
    if (PyErr_Occurred()) {
        return NULL;
    }
    Py_RETURN_NONE;
}

// Called once per child process
static int worker(int *a, int *b, int *c, unsigned start, unsigned end, unsigned mapped_size) {
    for (unsigned i = start; i < end; i++) {
        c[i] = a[i] * b[i];
    }

    // The child's mappings are its own (the memory behind them is shared), so it
    // unmaps all three. This does not affect the parent's mappings.
    int ret = 0;
    if (munmap(a, mapped_size) == -1) {
        perror("munmap");
        ret = -1;
    }
    if (munmap(b, mapped_size) == -1) {
        perror("munmap");
        ret = -1;
    }
    if (munmap(c, mapped_size) == -1) {
        perror("munmap");
        ret = -1;
    }
    return ret;
}
