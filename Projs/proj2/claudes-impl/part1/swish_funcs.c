#include "swish_funcs.h"

#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <unistd.h>

#include "string_vector.h"

#define MAX_ARGS 10

/*
 * Close the first 'n_fds' descriptors in 'fds'. Keeps going after a failure so
 * that one bad descriptor doesn't leak the rest.
 * Returns 0 if every close succeeded, -1 otherwise.
 */
static int close_fds(const int *fds, int n_fds) {
    int ret = 0;
    for (int i = 0; i < n_fds; i++) {
        if (close(fds[i]) == -1) {
            perror("close");
            ret = -1;
        }
    }
    return ret;
}

/*
 * Index of the last "|" token strictly before position 'end', or -1 if there
 * is none. Used to walk the pipeline from its last command to its first.
 */
static int find_pipe_before(const strvec_t *tokens, int end) {
    for (int i = end - 1; i >= 0; i--) {
        if (strcmp(strvec_get(tokens, i), "|") == 0) {
            return i;
        }
    }
    return -1;
}

/*
 * Returns 1 if any command in the pipeline has no tokens: a leading "|", a
 * trailing "|", or two adjacent "|". Returns 0 otherwise.
 */
static int has_empty_command(const strvec_t *tokens) {
    int prev_bar = -1;    // Index of the previous "|"; -1 stands for "before the first token"
    for (int i = 0; i < (int) tokens->length; i++) {
        if (strcmp(strvec_get(tokens, i), "|") == 0) {
            if (i == prev_bar + 1) {
                return 1;
            }
            prev_bar = i;
        }
    }
    return prev_bar == (int) tokens->length - 1;
}

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
 * Returns 0 on success or -1 on error.
 */
int run_piped_command(strvec_t *tokens, int *pipes, int n_pipe_fds, int in_idx, int out_idx) {
    if (in_idx != -1 && dup2(pipes[in_idx], STDIN_FILENO) == -1) {
        perror("dup2");
        close_fds(pipes, n_pipe_fds);
        return -1;
    }
    if (out_idx != -1 && dup2(pipes[out_idx], STDOUT_FILENO) == -1) {
        perror("dup2");
        close_fds(pipes, n_pipe_fds);
        return -1;
    }

    // stdin/stdout now refer to the right pipe ends, so every original pipe
    // descriptor can go, including the two just duplicated. A write end left
    // open here would keep the downstream reader from ever seeing EOF.
    if (close_fds(pipes, n_pipe_fds) == -1) {
        return -1;
    }

    // Applies any '<', '>' or '>>' itself (after our dup2, so a file redirect
    // wins), then execs. It only returns if something failed, and it has
    // already printed the error.
    run_command(tokens);
    return -1;
}

int run_pipelined_commands(strvec_t *tokens) {
    if (has_empty_command(tokens)) {
        fprintf(stderr, "Error: Empty command in pipeline\n");
        return -1;
    }

    int n_pipes = strvec_num_occurrences(tokens, "|");
    int n_cmds = n_pipes + 1;
    int n_pipe_fds = 2 * n_pipes;

    // Anything still sitting in stdout's buffer would be inherited by every
    // child, and a child that fails to exec flushes it again when it exits.
    if (fflush(stdout) == EOF) {
        perror("fflush");
        return -1;
    }

    // Pipe i connects command i (writes to pipes[2*i+1]) to command i+1
    // (reads from pipes[2*i]).
    int *pipes = malloc(n_pipe_fds * sizeof(int));
    if (pipes == NULL) {
        perror("malloc");
        return -1;
    }
    pid_t *pids = malloc(n_cmds * sizeof(pid_t));
    if (pids == NULL) {
        perror("malloc");
        free(pipes);
        return -1;
    }

    for (int i = 0; i < n_pipes; i++) {
        if (pipe(pipes + 2 * i) == -1) {
            perror("pipe");
            close_fds(pipes, 2 * i);    // Only the pipes created so far
            free(pipes);
            free(pids);
            return -1;
        }
    }

    // Launch from the last command to the first. 'end' is one past the last
    // token of the current command; the "|" before it marks where it starts.
    int ret = 0;
    int n_forked = 0;
    int end = tokens->length;
    for (int i = n_cmds - 1; i >= 0; i--) {
        int bar = find_pipe_before(tokens, end);    // -1 for the first command
        strvec_t cmd;
        if (strvec_slice(tokens, &cmd, bar + 1, end) == -1) {
            perror("strvec_slice");
            strvec_clear(&cmd);
            ret = -1;
            break;
        }

        pid_t pid = fork();
        if (pid == -1) {
            perror("fork");
            strvec_clear(&cmd);
            ret = -1;
            break;
        }

        if (pid == 0) {
            int in_idx = (i == 0) ? -1 : 2 * (i - 1);
            int out_idx = (i == n_cmds - 1) ? -1 : 2 * i + 1;
            run_piped_command(&cmd, pipes, n_pipe_fds, in_idx, out_idx);

            // Only reached if the command could not be started. This process
            // is a copy of the shell, so free its copies of the shell's heap
            // before exiting rather than returning into the shell's loop.
            strvec_clear(&cmd);
            strvec_clear(tokens);
            free(pipes);
            free(pids);
            exit(1);
        }

        pids[n_forked] = pid;
        n_forked++;
        strvec_clear(&cmd);
        end = bar;
    }

    // Close the parent's copies of every pipe end BEFORE waiting. While the
    // parent holds a write end, the reader on that pipe never sees EOF, so it
    // never exits, and waiting for it would hang the shell.
    if (close_fds(pipes, n_pipe_fds) == -1) {
        ret = -1;
    }
    free(pipes);

    // Reap every child that was started, even after a failed fork, so none is
    // left as a zombie. A child's own exit status does not change the result.
    for (int i = 0; i < n_forked; i++) {
        if (waitpid(pids[i], NULL, 0) == -1) {
            perror("waitpid");
            ret = -1;
        }
    }
    free(pids);

    return ret;
}
