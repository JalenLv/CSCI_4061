// redirect_child.c: starts a child process which will print into a
// file instead of onto the screen.
// Uses fork(), open(), dup2(), exec(), and wait()
#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/stat.h>
#include <sys/wait.h>
#include <unistd.h>

int main(int argc, char *argv[]) {
    if (argc < 2) {    // check for at least 1 command line arg
        printf("Usage: %s <childfile>\n", argv[0]);
        return 1;
    }

    // Uncomment lines below to use specified output file and command-line args in child process

    // output file that child process will print into
    // char *output_file = argv[1];

    // child command/arguments to execute
    // char *child_argv[] = {"wc", "test_cases/resources/nums.txt", NULL};

    // TODO: Spawn a child process, which will:
    //     open() the output file for writing
    //     Redirect standard output to the output file
    //     exec the "wc" command with the arguments in 'child_argv'

    // TODO: In the parent, wait for the child and ensure it terminated normally using wait macros
    // Print "Child complete, return code <status_code>" if child terminated normally, replacing
    //    <status_code> with the child's numerical status code
    // Print "Child exited abnormally" if child terminated abnormally

    return 0;
}
