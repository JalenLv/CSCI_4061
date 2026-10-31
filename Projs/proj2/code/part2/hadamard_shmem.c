#define _GNU_SOURCE

#include <Python.h>
#include <assert.h>
#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/mman.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <unistd.h>

#include "hadamard.h"

/*
 * Function executed in each child to compute its portion.

 * 'a': Pointer to shmem segment storing first matrix operand.
 * 'b': Pointer to shmem segment storing second matrix operand.
 * 'c': Pointer to shmem segment storing result matrix.
 *
 * 'start': Index (flattened) of first element of matrix portion
 * 'end': Index (flattened) of last element of matrix portion
 * 'mapped_size': Size (in bytes) of each shared memory segment
 *
 * Returns 0 on success or -1 on failure
 */
static int worker(int *a, int *b, int *c, unsigned start, unsigned end,
                  unsigned mapped_size) {
    /*
     * TODO Implement this function
     * Multiply each pair of elements from the assigned portion of the input
     * matrices and store in the result matrix.
     * Don't forget error handling and cleanup!
     */
    for (unsigned i = start; i < end; i++)
        c[i] = a[i] * b[i];

    if (munmap(a, mapped_size) || munmap(b, mapped_size) ||
        munmap(c, mapped_size)) {
        perror("munmap");
        return -1;
    }

    return 0;
}

PyObject *hadamard_shmem(PyObject *self, PyObject *args) {
    /*
     * TODO Implement this function
     * Read in arguments passed from Python
     * Read the two operand matrices from their files into shared memory
     * Set up shared memory segment to hold result matrix
     * Fork 'n_workers' children, giving each a roughly equal portion of the
     * input matrices to work on
     * Wait for all children to complete their work back in the parent
     * Then pass the result matrix to print_matrix()
     *
     * Don't forget error handling and cleanup!
     */
    const char *mat_a_file;
    const char *mat_b_file;
    unsigned n_procs;
    if (!PyArg_ParseTuple(args, "ssI", &mat_a_file, &mat_b_file, &n_procs)) {
        // ParseTuple sets up a Python exception on our behalf
        return NULL;
    }

    unsigned a_rows;
    unsigned a_cols;
    int *mat_a_data = read_matrix_from_file(mat_a_file, &a_rows, &a_cols, 1);
    if (mat_a_data == NULL) {
        PyErr_SetString(PyExc_OSError, "Failed to read matrix data from file");
        return NULL;
    }
    unsigned b_rows;
    unsigned b_cols;
    int *mat_b_data = read_matrix_from_file(mat_b_file, &b_rows, &b_cols, 1);
    if (mat_b_data == NULL) {
        PyErr_SetString(PyExc_OSError, "Failed to read matrix data from file");
        munmap(mat_a_data, a_rows * a_cols * sizeof(int));
        return NULL;
    }
    assert(a_rows == b_rows && a_cols == b_cols);

    unsigned c_rows = a_rows;
    unsigned c_cols = a_cols;
    unsigned c_size = c_rows * c_cols * sizeof(int);
    int *mat_c_data = mmap(NULL, c_size, PROT_READ | PROT_WRITE,
                           MAP_SHARED | MAP_ANONYMOUS, -1, 0);
    if (mat_c_data == MAP_FAILED) {
        PyErr_SetFromErrno(PyExc_OSError);
        munmap(mat_a_data, c_size);
        munmap(mat_b_data, c_size);
        return NULL;
    }

    pid_t *pids = malloc(n_procs * sizeof(pid_t));
    if (pids == NULL) {
        PyErr_NoMemory();
        munmap(mat_a_data, c_size);
        munmap(mat_b_data, c_size);
        munmap(mat_c_data, c_size);
        return NULL;
    }
    for (unsigned i = 0; i < n_procs; i++) {
        pid_t pid = fork();
        if (pid < 0) {
            PyErr_SetFromErrno(PyExc_OSError);
            munmap(mat_a_data, c_size);
            munmap(mat_b_data, c_size);
            munmap(mat_c_data, c_size);
            for (unsigned j = 0; j < i; j++)
                waitpid(pids[j], NULL, 0);
            free(pids);
            return NULL;
        }

        if (pid == 0) {
            // Child
            free(pids);

            unsigned ave_elems = ceil((c_rows * c_cols) / (double)n_procs);
            unsigned start = i * ave_elems;
            unsigned end = min(start + ave_elems, c_rows * c_cols);
            if (worker(mat_a_data, mat_b_data, mat_c_data, start, end, c_size))
                exit(1);
            exit(0);
        }

        // Parent
        pids[i] = pid;
    }

    for (unsigned i = 0; i < n_procs; i++) {
        int wstatus;
        if (waitpid(pids[i], &wstatus, 0) == -1) {
            PyErr_SetFromErrno(PyExc_OSError);
            munmap(mat_a_data, c_size);
            munmap(mat_b_data, c_size);
            munmap(mat_c_data, c_size);
            for (unsigned j = i + 1; j < n_procs; j++)
                waitpid(pids[j], NULL, 0);
            free(pids);
            return NULL;
        }
        if (!WIFEXITED(wstatus) || WEXITSTATUS(wstatus)) {
            PyErr_SetString(PyExc_OSError, "A child process failed");
            munmap(mat_a_data, c_size);
            munmap(mat_b_data, c_size);
            munmap(mat_c_data, c_size);
            for (unsigned j = i + 1; j < n_procs; j++)
                waitpid(pids[j], NULL, 0);
            free(pids);
            return NULL;
        }
    }
    free(pids);

    print_matrix(mat_c_data, c_rows, c_cols);

    if (munmap(mat_a_data, c_size) || munmap(mat_b_data, c_size) ||
        munmap(mat_c_data, c_size)) {
        PyErr_SetFromErrno(PyExc_OSError);
        return NULL;
    }
    Py_RETURN_NONE;
}
