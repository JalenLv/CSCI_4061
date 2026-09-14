#include <stdio.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <unistd.h>

int main() {
    // TODO Fork a child process to run the command "cat sample.txt"
    // The parent should wait and print out the child's exit code
    pid_t pid = fork();
    if (pid < 0) {
        perror("fork() failed");
        return 1;
    }

    int status;
    if (pid == 0) {
        execlp("/usr/bin/cat", "cat", "sample.txt", NULL);

        perror("execlp(\"cat\") failed");
        return 1;
    } else {
        if (wait(&status) == -1) {
            perror("wait() failed");
        }
    }

    printf("Child exited with status %d\n", WEXITSTATUS(status));
    return 0;
}
