#define _GNU_SOURCE
#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/mman.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <unistd.h>

#define NUMBERS_LEN 40
#define min(a, b)                                                              \
    ((a) < (b) ? (a)                                                           \
               : (b)) // You may find this useful when assigning work chunks

int numbers[NUMBERS_LEN] = {
    75, 3,  99, 36, 47, 88, 93, 93,  0,  96, 63, 27, 12, 67,
    75, 22, 34, 41, 61, 48, 13, 100, 86, 15, 87, 35, 19, 92,
    59, 70, 73, 65, 35, 35, 37, 44,  3,  20, 37, 27,
};

int main(int argc, char **argv) {
    if (argc < 2) {
        printf("Usage: %s <num_procs>\n", argv[0]);
        return 1;
    }

    // Uncomment the next line to use this variable definition
    int n_procs = atoi(argv[1]);

    // TODO Fork 'n_procs' worker processes, ensuring they have access to same
    // shared memory segment
    int *shared_numbers =
        mmap(NULL, NUMBERS_LEN * sizeof(int), PROT_READ | PROT_WRITE,
             MAP_SHARED | MAP_ANONYMOUS, -1, 0);
    if (shared_numbers == MAP_FAILED) {
        perror("mmap");
        return 1;
    }

    int n = NUMBERS_LEN / n_procs;
    int n_last = NUMBERS_LEN - (n * (n_procs - 1));
    for (int i = 0; i < n_procs; i++) {
        pid_t pid = fork();
        if (pid == 0) {
            // Assign each child a disjoint chunk of 'numbers' array to square
            // Chunks should be disjoint and (approximately) evenly sized
            int start = i * n;
            int end = start + ((i + 1 == n_procs) ? n_last : n);
            for (int j = start; j < end; j++)
                shared_numbers[j] = numbers[j] * numbers[j];

            if (munmap(shared_numbers, NUMBERS_LEN * sizeof(int)) != 0) {
                perror("munmap");
                return 1;
            }
            return 0;
        }
    }

    // Wait for all child processes to finish
    for (int i = 0; i < n_procs; i++)
        wait(NULL);

    printf("Squares:\n");
    for (int i = 0; i < NUMBERS_LEN; i++) {
        // TODO Print out results from shared mem segment
        printf("%d\n", shared_numbers[i]);
    }

    if (munmap(shared_numbers, NUMBERS_LEN * sizeof(int)) != 0) {
        perror("munmap");
        return 1;
    }
    return 0;
}
