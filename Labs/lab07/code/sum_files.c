#include <pthread.h>
#include <stdio.h>
#include <string.h>

#define N_THREADS 4

long global_sum;

long sum_file(const char *file_name) {
    FILE *f = fopen(file_name, "r");
    if (f == NULL) {
        perror("fopen");
        return -1;
    }

    long sum = 0;
    int current;
    while (fscanf(f, "%d", &current) != EOF) {
        sum += current;
    }
    if (ferror(f)) {
        perror("fscanf");
        fclose(f);
        return -1;
    }

    if (fclose(f) != 0) {
        perror("fclose");
        return -1;
    }

    return sum;
}

void *worker(void *arg) {
    // TODO Call 'sum_file' on the thread's assigned file name
    // Add the file's sum to 'global_sum'

    return NULL;
}

int main(int argc, char **argv) {
    // TODO Uncomment the variable definition below so you can use it
    /*
    char *file_names[] = {
        "test_cases/resources/numbers01.txt",
        "test_cases/resources/numbers02.txt",
        "test_cases/resources/numbers03.txt",
        "test_cases/resources/numbers04.txt",
    };
    */

    // TODO Create 4 threads and have each thread compute a sum for a different
    // element in the 'file_names' array

    // TODO Join all threads before printing out 'global_sum'

    printf("Sum for all files = %ld\n", global_sum);
}
