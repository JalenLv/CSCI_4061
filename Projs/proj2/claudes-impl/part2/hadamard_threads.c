#define _GNU_SOURCE

#include <Python.h>
#include <assert.h>
#include <errno.h>
#include <math.h>
#include <pthread.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "hadamard.h"

/*
 * Represents arguments for each thread
 * 'a': Pointer to first matrix operand
 * 'b': Pointer to second matrix operand
 * 'c': Pointer to result matrix
 * 'start': Index (in flattened representation) of first element of matrix portion
 * 'end': Index (in flattened representation) one past the last element of matrix portion
 */
struct thread_args {
    int *a;
    int *b;
    int *c;
    unsigned start;
    unsigned end;
};

/*
 * Function executed in each thread to compute its portion of the hadamard product
 * 'data': Pointer to struct thread_args instance
 * Declared here so this function can be referenced in hadamard_threads().
 * Complete the definition at the bottom of this file.
 * Returns NULL
 */
static void *worker(void *data);

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

// The provided read_matrix_from_file() sets a Python error itself in the original starter
// but only prints to stderr in the 10-22 update. Raise one if it didn't, without clobbering
// one it did; the message is the updated starter's own.
static void read_failed(void) {
    if (!PyErr_Occurred()) {
        PyErr_SetString(PyExc_OSError, "Failed to read matrix data from file");
    }
}

PyObject *hadamard_threads(PyObject *self, PyObject *args) {
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
    unsigned n_threads = (unsigned) n_workers;

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

    // Threads share the process's heap, so a plain malloc() is all C needs
    int *mat_c_data = malloc(n_elems * sizeof(int));
    pthread_t *threads = malloc(n_threads * sizeof(pthread_t));
    // One struct per thread: a thread may read its struct after this loop has moved on
    struct thread_args *thread_args = malloc(n_threads * sizeof(struct thread_args));
    if (mat_c_data == NULL || threads == NULL || thread_args == NULL) {
        free(mat_c_data);
        free(threads);
        free(thread_args);
        free(mat_a_data);
        free(mat_b_data);
        return PyErr_NoMemory();
    }

    unsigned n_started = 0;
    while (n_started < n_threads) {
        struct thread_args *t = &thread_args[n_started];
        t->a = mat_a_data;
        t->b = mat_b_data;
        t->c = mat_c_data;
        get_chunk(n_started, n_threads, n_elems, &t->start, &t->end);
        int ret = pthread_create(&threads[n_started], NULL, worker, t);
        if (ret != 0) {
            // pthread functions return an error number instead of setting errno
            errno = ret;
            PyErr_SetFromErrno(PyExc_OSError);
            break;    // Still join the threads that did start before cleaning up
        }
        n_started++;
    }

    for (unsigned i = 0; i < n_started; i++) {
        int ret = pthread_join(threads[i], NULL);
        // Keep the first error if one was already raised
        if (ret != 0 && !PyErr_Occurred()) {
            errno = ret;
            PyErr_SetFromErrno(PyExc_OSError);
        }
    }

    if (!PyErr_Occurred()) {
        print_matrix(mat_c_data, a_rows, a_cols);
    }
    free(mat_c_data);
    free(threads);
    free(thread_args);
    free(mat_a_data);
    free(mat_b_data);
    if (PyErr_Occurred()) {
        return NULL;
    }
    Py_RETURN_NONE;
}

void *worker(void *p) {
    const struct thread_args *args = p;
    // Each thread writes a disjoint range of C, so no locking is needed
    for (unsigned i = args->start; i < args->end; i++) {
        args->c[i] = args->a[i] * args->b[i];
    }
    return NULL;
}
