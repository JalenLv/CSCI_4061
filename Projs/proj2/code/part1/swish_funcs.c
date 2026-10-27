#include "swish_funcs.h"

#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <unistd.h>

#include "string_vector.h"

#define MAX_ARGS 10

/*
 * Helper function to run a single command within a pipeline. You should make
 * use of the provided 'run_command' function here.
 * tokens: String vector containing the tokens representing the command to be
 * executed, possible redirection, and the command's arguments.
 * pipes: An array of pipe file descriptors.
 * n_pipe_fds: Length of the 'pipes' array
 * in_idx: Index of the file descriptor in the array from which the program
 *         should read its input, or -1 if input should not be read from a pipe.
 * out_idx: Index of the file descriptor in the array to which the program
 *          should write its output, or -1 if output should not be written to
 *          a pipe.
 * Doesn't return on success but returns -1 on failure.
 */
int run_piped_command(strvec_t *tokens, int *pipes, int n_pipe_fds, int in_idx,
                      int out_idx) {
    if (in_idx != -1 && dup2(pipes[in_idx], STDIN_FILENO) == -1)
        goto piped_command_dup2_error;
    if (out_idx != -1 && dup2(pipes[out_idx], STDOUT_FILENO) == -1)
        goto piped_command_dup2_error;

    for (int i = 0; i < n_pipe_fds; i++)
        close(pipes[i]);

    run_command(tokens);
    return -1; // Unreachable if run_command is successful

piped_command_dup2_error:
    perror("dup2");
    for (int i = 0; i < n_pipe_fds; i++)
        close(pipes[i]);
    return -1;
}

/**
 * Checks the tokens for valid redirection operators and their placement
 * relative to pipes.
 */
static int check_redir(strvec_t *tokens) {
    int n_in_redir_ops = strvec_num_occurrences(tokens, "<");
    if (n_in_redir_ops > 1) {
        fprintf(stderr, "Error: Multiple input redir ops\n");
        return -1;
    }
    if (n_in_redir_ops == 1 &&
        strvec_find(tokens, "<") >= strvec_find(tokens, "|")) {
        fprintf(stderr, "Error: Input redir op after pipe\n");
        return -1;
    }

    int n_ovr_ops = strvec_num_occurrences(tokens, ">");
    int n_app_ops = strvec_num_occurrences(tokens, ">>");
    int n_out_redir_ops = n_ovr_ops + n_app_ops;
    if (n_out_redir_ops > 1) {
        fprintf(stderr, "Error: Multiple output redir ops\n");
        return -1;
    }
    if (n_out_redir_ops == 1) {
        char *token_tbf = (n_ovr_ops == 1) ? ">" : ">>";
        if (strvec_find(tokens, token_tbf) <= strvec_find_last(tokens, "|")) {
            fprintf(stderr, "Error: Output redir op before pipe\n");
            return -1;
        }
    }

    return 0;
}

/**
 * Splits the tokens into separate commands based on the pipe operator.
 * Side effects: Leaves only the first command in the tokens vector
 */
static int build_cmds(strvec_t *tokens, strvec_t **cmds, int *n_cmds) {
    *n_cmds = strvec_num_occurrences(tokens, "|") + 1;
    *cmds = malloc(*n_cmds * sizeof(strvec_t));
    if (*cmds == NULL) {
        perror("malloc");
        *n_cmds = 0;
        return -1;
    }

    for (int i = *n_cmds - 1; i >= 0; i--) {
        if (strvec_slice(tokens, *cmds + i, strvec_find_last(tokens, "|") + 1,
                         tokens->length) != 0) {
            fprintf(stderr, "Error: Failed to slice tokens\n");

            for (int j = i; j < *n_cmds; j++)
                strvec_clear(*cmds + j);
            free(*cmds);

            *n_cmds = 0;
            *cmds = NULL;
            return -1;
        }
        strvec_take(tokens, strvec_find_last(tokens, "|"));
    }

    return 0;
}

/**
 * Checks the commands to ensure that none of them are empty.
 */
static int check_cmds(strvec_t *cmds, int n_cmds) {
    for (int i = 0; i < n_cmds; i++) {
        if (cmds[i].length == 0) {
            fprintf(stderr, "Error: Empty command\n");
            return -1;
        }
    }
    return 0;
}

/**
 * Builds the pipes needed for the pipeline.
 */
static int build_pipes(int n_cmds, int **pipes, int *n_pipes) {
    *n_pipes = n_cmds - 1;
    *pipes = malloc((*n_pipes * 2) * sizeof(int));
    if (*pipes == NULL) {
        perror("malloc");
        *n_pipes = 0;
        return -1;
    }

    for (int i = 0; i < *n_pipes; i++) {
        if (pipe(*pipes + (i * 2)) != 0) {
            perror("pipe");

            for (int j = 0; j < i; j++) {
                close((*pipes)[j * 2]);
                close((*pipes)[j * 2 + 1]);
            }
            free(*pipes);

            *n_pipes = 0;
            *pipes = NULL;
            return -1;
        }
    }

    return 0;
}

/**
 * Cleans up the resources allocated for the pipeline, including
 * - commands,
 * - pipes,
 * - child processes.
 */
static void finish_pipeline(strvec_t *cmds, int n_cmds, int *pipes, int n_pipes,
                            pid_t *pids, int n_child_wait) {
    for (int i = 0; i < n_cmds; i++)
        strvec_clear(cmds + i);
    free(cmds);

    for (int i = 0; i < n_pipes; i++) {
        close(pipes[i * 2]);
        close(pipes[i * 2 + 1]);
    }
    free(pipes);

    for (int i = 0; i < n_child_wait; i++)
        waitpid(pids[i], NULL, 0);
    free(pids);
}

int run_pipelined_commands(strvec_t *tokens) {
    int n_cmds = 0;
    strvec_t *cmds = NULL;
    int n_pipes = 0;
    int *pipes = NULL;
    pid_t *pids = NULL;
    int n_child_wait = 0;
    int ret = -1;

    if (check_redir(tokens) != 0)
        goto piped_cmds_out;

    if (build_cmds(tokens, &cmds, &n_cmds) != 0)
        goto piped_cmds_out;

    if (check_cmds(cmds, n_cmds) != 0)
        goto piped_cmds_out;

    if (build_pipes(n_cmds, &pipes, &n_pipes) != 0)
        goto piped_cmds_out;

    pids = malloc(n_cmds * sizeof(pid_t));
    if (pids == NULL) {
        perror("malloc");
        goto piped_cmds_out;
    }

    // Flush stdout to ensure that the prompt is printed before any child
    // process output
    if (fflush(stdout) != 0) {
        perror("fflush");
        goto piped_cmds_out;
    }

    for (int i = 0; i < n_cmds; i++) {
        pid_t pid = fork();
        if (pid < 0) {
            perror("fork");
            goto piped_cmds_out;
        }

        if (pid == 0) {
            // Child process
            int in_idx = (i == 0) ? -1 : (i - 1) * 2;
            int out_idx = (i == n_cmds - 1) ? -1 : i * 2 + 1;
            run_piped_command(cmds + i, pipes, n_pipes * 2, in_idx, out_idx);

            n_pipes = 0; // Unreachable if run_piped_command is successful
            n_child_wait = 0;
            finish_pipeline(cmds, n_cmds, pipes, n_pipes, pids, n_child_wait);
            strvec_clear(tokens);
            exit(1);
        }

        // Parent process
        pids[n_child_wait++] = pid;
    }

    ret = 0;

piped_cmds_out:
    finish_pipeline(cmds, n_cmds, pipes, n_pipes, pids, n_child_wait);
    return ret;
}
