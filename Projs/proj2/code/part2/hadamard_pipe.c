#include <Python.h>
#include <assert.h>
#include <math.h>
#include <stdio.h>
#include <stdlib.h>
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
 * Function called by each child process to perform its portion of work
 * 'a': Pointer to elements for first matrix operand
 * 'b': Pointer to elements for second matrix operand
 * 'start': Index (in flattened representation) of first element of matrix portion
 * 'end': Index (in flattened representation) of last element of matrix portion
 * 'pipe_fd': Descriptor of write end of pipe to use to send results back to coordinator
 *
 * Returns 0 on success or -1 on failure
 */
// TODO: Uncomment this function definition. Implement the function,
// and call it from child processes.
// static int worker(const int *a, const int *b, unsigned start, unsigned end, int pipe_fd) {
//     return 0;
// }

PyObject *hadamard_pipe(PyObject *self, PyObject *args) {
    const char *mat_a_file;
    const char *mat_b_file;
    unsigned n_procs;
    if (!PyArg_ParseTuple(args, "ssI", &mat_a_file, &mat_b_file, &n_procs)) {
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

    /*
     * TODO Complete the implementation of this function
     * Create one pipe for all processes to share
     * Fork 'n_workers' child processes
     * Assign each child a roughly equal portion of matrix to work on
     * Set up a result matrix to store matrix elements sent by children
     * Collect results in the parent via the pipe
     * Don't forget cleanup and error handling!
     */

    Py_RETURN_NONE;
}
