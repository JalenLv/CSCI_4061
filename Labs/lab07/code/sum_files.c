#include <pthread.h>
#include <stdio.h>
#include <string.h>

#define N_THREADS 4

long global_sum;
pthread_mutex_t mutex = PTHREAD_MUTEX_INITIALIZER;

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
    char *file_name = (char *)arg;
    long file_sum = sum_file(file_name);
    if (file_sum == -1) {
        fprintf(stderr, "Error summing file: %s\n", file_name);
        return NULL;
    }

    // TODO use a mutex
    pthread_mutex_lock(&mutex);
    global_sum += file_sum;
    pthread_mutex_unlock(&mutex);

    return NULL;
}

int main(int argc, char **argv) {
    // TODO Uncomment the variable definition below so you can use it
    char *file_names[] = {
        "test_cases/resources/numbers01.txt",
        "test_cases/resources/numbers02.txt",
        "test_cases/resources/numbers03.txt",
        "test_cases/resources/numbers04.txt",
    };

    // TODO Create 4 threads and have each thread compute a sum for a different
    // element in the 'file_names' array
    pthread_t threads[N_THREADS];
    for (int i = 0; i < N_THREADS; i++) {
        int err = pthread_create(threads + i, NULL, worker, file_names[i]);
        if (err != 0) {
            fprintf(stderr, "pthread_create: %s\n", strerror(err));
            return 1;
        }
    }

    // TODO Join all threads before printing out 'global_sum'
    for (int i = 0; i < N_THREADS; i++) {
        int err = pthread_join(threads[i], NULL);
        if (err != 0) {
            fprintf(stderr, "pthread_join: %s\n", strerror(err));
            return 1;
        }
    }

    printf("Sum for all files = %ld\n", global_sum);
    return 0;
}
