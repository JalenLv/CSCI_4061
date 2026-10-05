#include <stdio.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <unistd.h>

int main() {
    // Run equivalent of 'sort -n test_cases/resources/numbers.txt | tail -n 10'

    int pipe_fds[2];
    if (pipe(pipe_fds) != 0) {
        perror("pipe");
        return 1;
    }

    pid_t child_pid = fork();
    if (child_pid == -1) {
        perror("fork");
        // TODO Insert any necessary cleanup
        for (int i = 0; i < 2; i++)
            if (close(pipe_fds[i]) != 0)
                perror("close");
        return 1;
    } else if (child_pid == 0) {
        // TODO Close read end of pipe
        if (close(pipe_fds[0]) != 0) {
            perror("close");
            goto CHILD_CLEANUP;
        }

        // TODO Run the 'sort' command, redirecting output to pipe first
        if (dup2(pipe_fds[1], STDOUT_FILENO) == -1) {
            perror("dup2");
            goto CHILD_CLEANUP;
        }

        char *const args[] = {"sort", "-n", "test_cases/resources/numbers.txt",
                              NULL};
        execvp(args[0], args);
        perror("exec");
        return 1;

    CHILD_CLEANUP:
        if (close(pipe_fds[1]) != 0)
            perror("close");
        return 1; // Not reached on successful exec()
    }

    // This code only reached by the parent
    // TODO Parent closes write end of pipe
    if (close(pipe_fds[1]) != 0) {
        perror("close");
        goto PARENT_CLEANUP;
    }

    // TODO Run 'tail' command in original process, redirecting input from pipe
    // first
    if (dup2(pipe_fds[0], STDIN_FILENO) == -1) {
        perror("dup2");
        goto PARENT_CLEANUP;
    }

    char *const args[] = {"tail", "-n", "10", NULL};
    execvp(args[0], args);
    perror("exec");
    return 1;

PARENT_CLEANUP:
    if (close(pipe_fds[0]) != 0)
        perror("close");
    return 1; // Not reached on successful exec()
}
