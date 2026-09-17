#define _GNU_SOURCE

#include "swish_funcs.h"

#include <assert.h>
#include <fcntl.h>
#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <unistd.h>

#include "job_list.h"
#include "string_vector.h"

#define MAX_ARGS 10

/*
 * Helper for 'fg', 'bg', and 'wait-for': all three take a job list index as
 * their single argument. Parses that index out of token 1 and looks up the
 * corresponding job.
 * tokens: Tokens from the command typed in by the user, e.g., "fg 0"
 * jobs: The list of current jobs for the shell
 * idx_out: Set to the parsed index on success (needed by callers that must
 *          later remove the job from the list)
 * Returns a pointer to the job on success, or NULL on error (an error message
 * has already been printed in that case)
 */
static job_t *lookup_job_arg(strvec_t *tokens, job_list_t *jobs, int *idx_out) {
    const char *idx_str = strvec_get(tokens, 1);
    if (idx_str == NULL) {
        fprintf(stderr, "Missing job index argument\n");
        return NULL;
    }

    int idx;
    // sscanf() returns the number of items successfully converted
    if (sscanf(idx_str, "%d", &idx) != 1 || idx < 0) {
        fprintf(stderr, "Job index out of bounds\n");
        return NULL;
    }

    job_t *job = job_list_get(jobs, idx);
    if (job == NULL) {
        fprintf(stderr, "Job index out of bounds\n");
        return NULL;
    }

    *idx_out = idx;
    return job;
}

/*
 * Task 3 helper: Set up input redirection ('<') for this process.
 * tokens: Vector containing tokens input by user into shell
 * op_idx: Index of the '<' token within 'tokens'
 * Returns 0 on success or -1 on error
 */
static int redirect_input(strvec_t *tokens, int op_idx) {
    const char *file_name = strvec_get(tokens, op_idx + 1);
    if (file_name == NULL) {
        fprintf(stderr, "Missing file name for input redirection\n");
        return -1;
    }

    int fd = open(file_name, O_RDONLY);
    if (fd == -1) {
        perror("Failed to open input file");
        return -1;
    }

    if (dup2(fd, STDIN_FILENO) == -1) {
        perror("dup2");
        close(fd);
        return -1;
    }
    // The duplicate in STDIN_FILENO keeps the file open, so this one is no longer needed
    if (close(fd) == -1) {
        perror("close");
        return -1;
    }

    return 0;
}

/*
 * Task 3 helper: Set up output redirection ('>' or '>>') for this process.
 * tokens: Vector containing tokens input by user into shell
 * op_idx: Index of the '>' or '>>' token within 'tokens'
 * append: 1 if output should be appended to the file ('>>'), 0 if the file
 *         should be truncated ('>')
 * Returns 0 on success or -1 on error
 */
static int redirect_output(strvec_t *tokens, int op_idx, int append) {
    const char *file_name = strvec_get(tokens, op_idx + 1);
    if (file_name == NULL) {
        fprintf(stderr, "Missing file name for output redirection\n");
        return -1;
    }

    int flags = O_WRONLY | O_CREAT | (append ? O_APPEND : O_TRUNC);
    int fd = open(file_name, flags, S_IRUSR | S_IWUSR);
    if (fd == -1) {
        perror("Failed to open output file");
        return -1;
    }

    if (dup2(fd, STDOUT_FILENO) == -1) {
        perror("dup2");
        close(fd);
        return -1;
    }
    if (close(fd) == -1) {
        perror("close");
        return -1;
    }

    return 0;
}

int tokenize(char *s, strvec_t *tokens) {
    // Task 0: Split 's' on single spaces, adding each token to 'tokens'
    char *token = strtok(s, " ");
    while (token != NULL) {
        if (strvec_add(tokens, token) == -1) {
            fprintf(stderr, "Failed to add token to token vector\n");
            return -1;
        }
        // Passing NULL continues tokenizing the string from the previous call
        token = strtok(NULL, " ");
    }
    return 0;
}

int run_command(strvec_t *tokens) {
    // Task 4: Children should not inherit the shell's decision to ignore
    // SIGTTIN and SIGTTOU, so restore the default handlers for both
    struct sigaction sac;
    sac.sa_handler = SIG_DFL;
    if (sigfillset(&sac.sa_mask) == -1) {
        perror("sigfillset");
        return -1;
    }
    sac.sa_flags = 0;
    if (sigaction(SIGTTIN, &sac, NULL) == -1 || sigaction(SIGTTOU, &sac, NULL) == -1) {
        perror("sigaction");
        return -1;
    }

    // Task 4: Place this process in a new process group of its own so that the
    // terminal can deliver signals (e.g., from Ctrl-C) to it without also
    // hitting the shell process
    pid_t pid = getpid();
    if (setpgid(pid, pid) == -1) {
        perror("setpgid");
        return -1;
    }

    // Task 3: Redirection operators and their operands are not arguments to the
    // program, so the argument list ends at the first one that appears
    int in_idx = strvec_find(tokens, "<");
    int trunc_idx = strvec_find(tokens, ">");
    int append_idx = strvec_find(tokens, ">>");

    unsigned arg_count = tokens->length;
    if (in_idx != -1 && (unsigned) in_idx < arg_count) {
        arg_count = in_idx;
    }
    if (trunc_idx != -1 && (unsigned) trunc_idx < arg_count) {
        arg_count = trunc_idx;
    }
    if (append_idx != -1 && (unsigned) append_idx < arg_count) {
        arg_count = append_idx;
    }

    if (arg_count == 0) {
        fprintf(stderr, "No command specified\n");
        return -1;
    }
    if (arg_count > MAX_ARGS) {
        fprintf(stderr, "Too many arguments: at most %d are supported\n", MAX_ARGS);
        return -1;
    }

    if (in_idx != -1 && redirect_input(tokens, in_idx) == -1) {
        return -1;
    }
    // The user gives at most one of '>' or '>>', never both
    if (trunc_idx != -1 && redirect_output(tokens, trunc_idx, 0) == -1) {
        return -1;
    }
    if (append_idx != -1 && redirect_output(tokens, append_idx, 1) == -1) {
        return -1;
    }

    // Task 2: Build the argument array exec() expects. MAX_ARGS tokens can make
    // it up, plus one more slot for the NULL sentinel exec() looks for
    char *args[MAX_ARGS + 1];
    for (unsigned i = 0; i < arg_count; i++) {
        args[i] = strvec_get(tokens, i);
    }
    args[arg_count] = NULL;

    // Using execvp() rather than execv() so that PATH is searched for the program
    execvp(args[0], args);
    // exec() only returns if it failed
    perror("exec");
    return -1;
}

int resume_job(strvec_t *tokens, job_list_t *jobs, int is_foreground) {
    int idx;
    job_t *job = lookup_job_arg(tokens, jobs, &idx);
    if (job == NULL) {
        return -1;
    }
    pid_t job_pid = job->pid;

    // Task 5: A job resumed in the foreground takes over the terminal, so that
    // keyboard signals reach it rather than the shell. There is only a terminal
    // to take over if stdin is one, which it isn't when input has been piped in
    int has_terminal = isatty(STDIN_FILENO);
    if (is_foreground && has_terminal && tcsetpgrp(STDIN_FILENO, job_pid) == -1) {
        perror("tcsetpgrp");
        return -1;
    }

    // The job is stopped, so it needs SIGCONT to start running again
    if (kill(job_pid, SIGCONT) == -1) {
        perror("kill");
        if (is_foreground && has_terminal && tcsetpgrp(STDIN_FILENO, getpid()) == -1) {
            perror("tcsetpgrp");
        }
        return -1;
    }

    // Task 6: A job resumed in the background is neither moved into the
    // foreground nor waited for, it is just marked as running again
    if (!is_foreground) {
        job->status = BACKGROUND;
        return 0;
    }

    int retval = 0;
    int status;
    // WUNTRACED so that waitpid() also returns if the job is stopped again
    if (waitpid(job_pid, &status, WUNTRACED) == -1) {
        perror("waitpid");
        retval = -1;
    } else if (!WIFSTOPPED(status)) {
        // The job is gone for good, so it no longer belongs in the jobs list
        if (job_list_remove(jobs, idx) == -1) {
            fprintf(stderr, "Failed to remove job from job list\n");
            retval = -1;
        }
    }

    // Task 5: Put the shell back in the foreground of the terminal
    if (has_terminal && tcsetpgrp(STDIN_FILENO, getpid()) == -1) {
        perror("tcsetpgrp");
        retval = -1;
    }

    return retval;
}

int await_background_job(strvec_t *tokens, job_list_t *jobs) {
    int idx;
    job_t *job = lookup_job_arg(tokens, jobs, &idx);
    if (job == NULL) {
        return -1;
    }

    // A stopped job will never terminate on its own, so waiting on it would
    // block the shell forever
    if (job->status != BACKGROUND) {
        fprintf(stderr, "Job index is for stopped process not background process\n");
        return -1;
    }

    int status;
    if (waitpid(job->pid, &status, WUNTRACED) == -1) {
        perror("waitpid");
        return -1;
    }

    if (WIFSTOPPED(status)) {
        // The job is no longer running in the background, it is now suspended
        job->status = STOPPED;
    } else if (job_list_remove(jobs, idx) == -1) {
        fprintf(stderr, "Failed to remove job from job list\n");
        return -1;
    }

    return 0;
}

int await_all_background_jobs(job_list_t *jobs) {
    int retval = 0;

    // Removing entries while walking the list would invalidate 'current', so
    // the terminated jobs are all cleaned up afterwards instead
    for (job_t *current = jobs->head; current != NULL; current = current->next) {
        if (current->status != BACKGROUND) {
            continue;
        }

        int status;
        if (waitpid(current->pid, &status, WUNTRACED) == -1) {
            perror("waitpid");
            retval = -1;
        } else if (WIFSTOPPED(status)) {
            // Marking it STOPPED both records its new state and protects it
            // from the cleanup below
            current->status = STOPPED;
        }
    }

    // Everything still marked BACKGROUND terminated in the loop above
    job_list_remove_by_status(jobs, BACKGROUND);
    return retval;
}
