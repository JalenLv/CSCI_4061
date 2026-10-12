#define _GNU_SOURCE
#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/mman.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <unistd.h>

#define NUMBERS_LEN 40
#define min(a, b) ((a) < (b) ? (a) : (b))    // You may find this useful when assigning work chunks

int numbers[NUMBERS_LEN] = {
    75, 3,   99, 36, 47, 88, 93, 93, 0,  96, 63, 27, 12, 67, 75, 22, 34, 41, 61, 48,
    13, 100, 86, 15, 87, 35, 19, 92, 59, 70, 73, 65, 35, 35, 37, 44, 3,  20, 37, 27,
};

int main(int argc, char **argv) {
    if (argc < 2) {
        printf("Usage: %s <num_procs>\n", argv[0]);
        return 1;
    }

    // Uncomment the next line to use this variable definition
    // int n_procs = atoi(argv[1]);

    // TODO Fork 'n_procs' worker processes, ensuring they have access to same
    // shared memory segment

    // Assign each child a disjoint chunk of 'numbers' array to square
    // Chunks should be disjoint and (approximately) evenly sized

    printf("Squares:\n");
    for (int i = 0; i < NUMBERS_LEN; i++) {
        // TODO Print out results from shared mem segment
    }

    return 0;
}
