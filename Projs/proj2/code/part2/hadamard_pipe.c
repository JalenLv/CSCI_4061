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
 * 'start': Index of first element of (flattened) matrix portion
 * 'end': Index of last element of (flattened) matrix portion
 * 'pipe_fd': Fd of write end of pipe to use to send results back to coordinator
 *
 * Returns 0 on success or -1 on failure
 */
static int worker(const int *a, const int *b, unsigned start, unsigned end,
                  int pipe_fd) {
    for (unsigned i = start; i < end; i++) {
        struct element result = {i, a[i] * b[i]};
        if (write(pipe_fd, &result, sizeof(result)) == -1) {
            perror("write");
            return -1;
        }
    }
    return 0;
}

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
    int pipe_fds[2];
    if (pipe(pipe_fds) == -1) {
        PyErr_SetFromErrno(PyExc_OSError);
        free(mat_a_data);
        free(mat_b_data);
        return NULL;
    }

    pid_t *pids = malloc(n_procs * sizeof(pid_t));
    if (pids == NULL) {
        PyErr_NoMemory();
        free(mat_a_data);
        free(mat_b_data);
        close(pipe_fds[0]);
        close(pipe_fds[1]);
        return NULL;
    }
    int n_child_wait = 0;
    for (unsigned i = 0; i < n_procs; i++) {
        pid_t pid = fork();
        if (pid < 0) {
            PyErr_SetFromErrno(PyExc_OSError);
            free(mat_a_data);
            free(mat_b_data);
            close(pipe_fds[0]);
            close(pipe_fds[1]);
            for (int j = 0; j < n_child_wait; j++)
                waitpid(pids[j], NULL, 0);
            free(pids);
            return NULL;
        }

        if (pid == 0) {
            // Child
            close(pipe_fds[0]);

            unsigned ave_elems = ceil((a_rows * a_cols) / (double)n_procs);
            unsigned start = i * ave_elems;
            unsigned end = min(start + ave_elems, a_rows * a_cols);
            int exit_code = 0;
            if (worker(mat_a_data, mat_b_data, start, end, pipe_fds[1]))
                exit_code = 1;

            close(pipe_fds[1]);
            free(mat_a_data);
            free(mat_b_data);
            // A child must not wait its siblings
            free(pids);
            exit(exit_code);
        }

        // Parent
        pids[n_child_wait++] = pid;
    }

    // Parent
    close(pipe_fds[1]);

    unsigned c_rows = a_rows;
    unsigned c_cols = a_cols;
    int *mat_c_data = malloc(c_rows * c_cols * sizeof(int));
    if (mat_c_data == NULL) {
        PyErr_NoMemory();
        close(pipe_fds[0]);
        free(mat_a_data);
        free(mat_b_data);
        for (int i = 0; i < n_child_wait; i++)
            waitpid(pids[i], NULL, 0);
        free(pids);
        return NULL;
    }

    struct element ele;
    int nbytes;
    while ((nbytes = read(pipe_fds[0], &ele, sizeof(ele))) > 0)
        mat_c_data[ele.idx] = ele.value;

    if (nbytes) {
        PyErr_SetFromErrno(PyExc_OSError);
        close(pipe_fds[0]);
        free(mat_a_data);
        free(mat_b_data);
        free(mat_c_data);
        for (int i = 0; i < n_child_wait; i++)
            waitpid(pids[i], NULL, 0);
        free(pids);
        return NULL;
    }

    // Wait before printing the result matrix to ensure all child processes have
    // completed successfully
    for (int i = 0; i < n_child_wait; i++) {
        int wstatus;
        if (waitpid(pids[i], &wstatus, 0) == -1) {
            PyErr_SetFromErrno(PyExc_OSError);
            close(pipe_fds[0]);
            free(mat_a_data);
            free(mat_b_data);
            free(mat_c_data);
            for (int j = i + 1; j < n_child_wait; j++)
                waitpid(pids[j], NULL, 0);
            free(pids);
            return NULL;
        }
        if (!WIFEXITED(wstatus) || WEXITSTATUS(wstatus)) {
            PyErr_SetString(PyExc_OSError, "A child process failed");
            close(pipe_fds[0]);
            free(mat_a_data);
            free(mat_b_data);
            free(mat_c_data);
            for (int j = i + 1; j < n_child_wait; j++)
                waitpid(pids[j], NULL, 0);
            free(pids);
            return NULL;
        }
    }
    free(pids);

    print_matrix(mat_c_data, c_rows, c_cols);

    close(pipe_fds[0]);
    free(mat_a_data);
    free(mat_b_data);
    free(mat_c_data);
    Py_RETURN_NONE;
}
