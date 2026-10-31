#define _GNU_SOURCE

#include <Python.h>
#include <assert.h>
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
 * 'start': Index (in flattened representation) of first element of matrix
 * portion 'end': Index (in flattened representation) of second element of
 * matrix portion
 */
struct thread_args {
    int *a;
    int *b;
    int *c;
    unsigned start;
    unsigned end;
};

/*
 * Function executed in each thread to compute its portion of the hadamard
 * product 'data': Pointer to struct thread_args instance Declared here so this
 * function can be referenced in hadamard_threads(). Complete the definition at
 * the bottom of this file. Returns NULL
 */
static void *worker(void *data) {
    /*
     * TODO Complete this function
     * Multiply each pair of elements and store the result, for the assigned
     * portion of the input matrices.
     */
    struct thread_args *args = (struct thread_args *)data;
    for (unsigned i = args->start; i < args->end; i++)
        args->c[i] = args->a[i] * args->b[i];
    return NULL;
}

PyObject *hadamard_threads(PyObject *self, PyObject *args) {
    /*
     * TODO Complete this function
     * Read arguments passed in by Python
     * Read the two input matrices from their files
     * Set up a result matrix on the heap
     * Launch 'n_workers' threads, each assigned a roughly equal portion of the
     * matrices to work on.
     * Wait for all threads to complete, then pass the result matrix to the
     * print_matrix() function.
     * Don't forget error handling and cleanup!
     */
    const char *mat_a_file;
    const char *mat_b_file;
    unsigned n_workers;
    if (!PyArg_ParseTuple(args, "ssI", &mat_a_file, &mat_b_file, &n_workers)) {
        // ParseTuple sets up a Python exception on our behalf
        return NULL;
    }

    unsigned a_rows;
    unsigned a_cols;
    int *mat_a_data = read_matrix_from_file(mat_a_file, &a_rows, &a_cols, 0);
    if (mat_a_data == NULL) {
        PyErr_SetString(PyExc_OSError, "Failed to read matrix data from file");
        return NULL;
    }
    unsigned b_rows;
    unsigned b_cols;
    int *mat_b_data = read_matrix_from_file(mat_b_file, &b_rows, &b_cols, 0);
    if (mat_b_data == NULL) {
        PyErr_SetString(PyExc_OSError, "Failed to read matrix data from file");
        free(mat_a_data);
        return NULL;
    }
    assert(a_rows == b_rows && a_cols == b_cols);

    unsigned c_rows = a_rows;
    unsigned c_cols = a_cols;
    int *mat_c_data = malloc(c_rows * c_cols * sizeof(int));
    if (mat_c_data == NULL) {
        PyErr_NoMemory();
        free(mat_a_data);
        free(mat_b_data);
        return NULL;
    }

    pthread_t *threads = malloc(n_workers * sizeof(pthread_t));
    if (threads == NULL) {
        PyErr_NoMemory();
        free(mat_a_data);
        free(mat_b_data);
        free(mat_c_data);
        return NULL;
    }
    struct thread_args *data = malloc(n_workers * sizeof(struct thread_args));
    if (data == NULL) {
        PyErr_NoMemory();
        free(mat_a_data);
        free(mat_b_data);
        free(mat_c_data);
        free(threads);
        return NULL;
    }
    for (unsigned i = 0; i < n_workers; i++) {
        unsigned ave_elems = ceil((c_rows * c_cols) / (double)n_workers);
        unsigned start = i * ave_elems;
        unsigned end = min(start + ave_elems, c_rows * c_cols);
        data[i] = (struct thread_args){.a = mat_a_data,
                                       .b = mat_b_data,
                                       .c = mat_c_data,
                                       .start = start,
                                       .end = end};
        int err = pthread_create(&threads[i], NULL, worker, &data[i]);
        if (err) {
            PyErr_Format(PyExc_OSError, "pthread_create: %s", strerror(err));
            for (unsigned j = 0; j < i; j++)
                pthread_join(threads[j], NULL);
            free(threads);
            free(mat_a_data);
            free(mat_b_data);
            free(mat_c_data);
            free(data);
            return NULL;
        }
    }
    for (unsigned i = 0; i < n_workers; i++) {
        int err = pthread_join(threads[i], NULL);
        if (err) {
            PyErr_Format(PyExc_OSError, "pthread_join: %s", strerror(err));
            for (unsigned j = i + 1; j < n_workers; j++)
                pthread_join(threads[j], NULL);
            free(mat_a_data);
            free(mat_b_data);
            free(mat_c_data);
            free(threads);
            free(data);
            return NULL;
        }
    }

    print_matrix(mat_c_data, c_rows, c_cols);

    free(mat_a_data);
    free(mat_b_data);
    free(mat_c_data);
    free(threads);
    free(data);
    Py_RETURN_NONE;
}
