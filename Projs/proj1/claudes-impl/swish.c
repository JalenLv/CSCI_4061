#define _GNU_SOURCE

#include <errno.h>
#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <unistd.h>

#include "job_list.h"
#include "string_vector.h"
#include "swish_funcs.h"

#define CMD_LEN 512
#define PROMPT "@> "

int main(int argc, char **argv) {
    // Task 4: Set up shell to ignore SIGTTIN, SIGTTOU when put in background
    // You should adapt this code for use in run_command().
    struct sigaction sac;
    sac.sa_handler = SIG_IGN;
    if (sigfillset(&sac.sa_mask) == -1) {
        perror("sigfillset");
        return 1;
    }
    sac.sa_flags = 0;
    if (sigaction(SIGTTIN, &sac, NULL) == -1 || sigaction(SIGTTOU, &sac, NULL) == -1) {
        perror("sigaction");
        return 1;
    }

    strvec_t tokens;
    if (strvec_init(&tokens) == -1) {
        fprintf(stderr, "Failed to initialize token vector\n");
        return 1;
    }
    job_list_t jobs;
    job_list_init(&jobs);
    char cmd[CMD_LEN];

    printf("%s", PROMPT);
    while (fgets(cmd, CMD_LEN, stdin) != NULL) {
        // Need to remove trailing '\n' from cmd. There are fancier ways.
        int i = 0;
        while (cmd[i] != '\n' && cmd[i] != '\0') {
            i++;
        }
        cmd[i] = '\0';

        if (tokenize(cmd, &tokens) != 0) {
            printf("Failed to parse command\n");
            strvec_clear(&tokens);
            job_list_free(&jobs);
            return 1;
        }
        if (tokens.length == 0) {
            printf("%s", PROMPT);
            continue;
        }
        const char *first_token = strvec_get(&tokens, 0);

        if (strcmp(first_token, "pwd") == 0) {
            // Task 1: Print the shell's current working directory
            char cwd[CMD_LEN];
            if (getcwd(cwd, CMD_LEN) == NULL) {
                perror("getcwd");
            } else {
                printf("%s\n", cwd);
            }
        }

        else if (strcmp(first_token, "cd") == 0) {
            // Task 1: Change the shell's current working directory. With no
            // argument, fall back to the user's home directory
            const char *dir = strvec_get(&tokens, 1);
            if (dir == NULL) {
                dir = getenv("HOME");
            }
            if (dir == NULL) {
                fprintf(stderr, "Failed to look up HOME directory\n");
            } else if (chdir(dir) == -1) {
                perror("chdir");
            }
        }

        else if (strcmp(first_token, "exit") == 0) {
            strvec_clear(&tokens);
            break;
        }

        // Task 5: Print out current list of pending jobs
        else if (strcmp(first_token, "jobs") == 0) {
            int i = 0;
            job_t *current = jobs.head;
            while (current != NULL) {
                char *status_desc;
                if (current->status == BACKGROUND) {
                    status_desc = "background";
                } else {
                    status_desc = "stopped";
                }
                printf("%d: %s (%s)\n", i, current->name, status_desc);
                i++;
                current = current->next;
            }
        }

        // Task 5: Move stopped job into foreground
        else if (strcmp(first_token, "fg") == 0) {
            if (resume_job(&tokens, &jobs, 1) == -1) {
                printf("Failed to resume job in foreground\n");
            }
        }

        // Task 6: Move stopped job into background
        else if (strcmp(first_token, "bg") == 0) {
            if (resume_job(&tokens, &jobs, 0) == -1) {
                printf("Failed to resume job in background\n");
            }
        }

        // Task 6: Wait for a specific job identified by its index in job list
        else if (strcmp(first_token, "wait-for") == 0) {
            if (await_background_job(&tokens, &jobs) == -1) {
                printf("Failed to wait for background job\n");
            }
        }

        // Task 6: Wait for all background jobs
        else if (strcmp(first_token, "wait-all") == 0) {
            if (await_all_background_jobs(&jobs) == -1) {
                printf("Failed to wait for all background jobs\n");
            }
        }

        else {
            // Task 6: A trailing "&" means the command runs in the background.
            // It is not part of the command itself, so drop it from the tokens
            int is_background = strcmp(strvec_get(&tokens, tokens.length - 1), "&") == 0;
            if (is_background) {
                strvec_take(&tokens, tokens.length - 1);
            }

            if (tokens.length == 0) {
                // The user typed nothing but "&", so there is no program to run
                fprintf(stderr, "No command specified\n");
                strvec_clear(&tokens);
                printf("%s", PROMPT);
                continue;
            }

            // The prompt printed above is still sitting in stdout's buffer if
            // stdout isn't a terminal. Flushing now keeps the child from
            // inheriting (and later re-printing) a copy of it
            fflush(stdout);

            pid_t child_pid = fork();
            if (child_pid == -1) {
                perror("fork");
            }

            else if (child_pid == 0) {
                // Task 2: The child becomes the user's program. run_command()
                // only returns if that failed, and it has already reported why,
                // so all this child can do is clean up and exit
                if (run_command(&tokens) == -1) {
                    strvec_clear(&tokens);
                    job_list_free(&jobs);
                    return 1;
                }
            }

            else {
                // Task 4: run_command() puts the child in its own process
                // group, but the parent races ahead to tcsetpgrp() below.
                // Setting the group here too means it exists either way.
                // EACCES means the child already exec'd (so it set its own
                // group first), ESRCH that it has already terminated
                if (setpgid(child_pid, child_pid) == -1 && errno != EACCES && errno != ESRCH) {
                    perror("setpgid");
                }

                if (is_background) {
                    // Task 6: Don't touch the terminal and don't wait: the job
                    // runs alongside the shell until the user asks about it
                    if (job_list_add(&jobs, child_pid, strvec_get(&tokens, 0), BACKGROUND) == -1) {
                        fprintf(stderr, "Failed to add job to job list\n");
                    }
                } else {
                    // Task 4: Hand the terminal to the child so that keyboard
                    // signals such as Ctrl-C reach it instead of the shell.
                    // There is only a terminal to hand over if stdin is one,
                    // which it isn't when the shell's input has been piped in
                    int has_terminal = isatty(STDIN_FILENO);
                    if (has_terminal && tcsetpgrp(STDIN_FILENO, child_pid) == -1) {
                        perror("tcsetpgrp");
                    }

                    // Task 5: WUNTRACED so waitpid() returns for a child that
                    // was stopped (Ctrl-Z) as well as one that terminated
                    int status;
                    if (waitpid(child_pid, &status, WUNTRACED) == -1) {
                        perror("waitpid");
                    } else if (WIFSTOPPED(status)) {
                        // The process is still alive, so remember it to let the
                        // user resume it later with 'fg' or 'bg'
                        if (job_list_add(&jobs, child_pid, strvec_get(&tokens, 0), STOPPED) == -1) {
                            fprintf(stderr, "Failed to add job to job list\n");
                        }
                    }

                    // Task 4: Take the terminal back for the shell
                    if (has_terminal && tcsetpgrp(STDIN_FILENO, getpid()) == -1) {
                        perror("tcsetpgrp");
                    }
                }
            }
        }

        strvec_clear(&tokens);
        printf("%s", PROMPT);
    }

    // Handles the case where the shell read EOF before any command was run,
    // leaving the vector initialized but never cleared
    strvec_clear(&tokens);
    job_list_free(&jobs);
    return 0;
}
