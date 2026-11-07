// SPDX-FileCopyrightText: 2022 -- 2026 John Kolb, Matthew Dorow
// SPDX-License-Identifier: GPL-3.0-or-later
#define _GNU_SOURCE

#include <dlfcn.h>
#include <fcntl.h>
#include <pthread.h>
#include <semaphore.h>
#include <stdio.h>
#include <string.h>

#define SERVER_FILE_INDICATOR "server_files/"
#define CONCURRENCY_DEGREE 5

static pthread_mutex_t lock = PTHREAD_MUTEX_INITIALIZER;
static int n_waiters = 0;
static sem_t semaphore;

/*
 * Versions of various open functions that will only allow threads to proceed
 * once a sufficient number of threads have initiated an open call
 * Specifically, it blocks all calling threads until 'CONCURRENCY_DEGREE'
 * threads have made a call to one of the open functions.
 * This is a (probably inelegant) way to check if a program is really capable
 * of 'CONCURRENCY_DEGREE' threads of execution.
 */

// Runs at library initialization time
__attribute__((constructor)) static void init_semaphore(void) {
    if (sem_init(&semaphore, 0, 0) == -1) {
        perror("sem_init");
    }
}

// Returns true if the pathname provided is to a server file, false otherwise
int is_server_file(const char *pathname) {
    return strstr(pathname, SERVER_FILE_INDICATOR) != NULL;
}

// Wait until 'CONCURRENCY_DEGREE' threads all have initiated barrier(). Then,
// allow all of them to proceed.
int barrier(void) {
    int result;
    if ((result = pthread_mutex_lock(&lock)) != 0) {
        fprintf(stderr, "pthread_mutex_lock: %s\n", strerror(result));
        return -1;
    }
    if (n_waiters == CONCURRENCY_DEGREE - 1) {
        for (int i = 0; i < CONCURRENCY_DEGREE - 1; i++) {
            if (sem_post(&semaphore) == -1) {
                perror("sem_post");
                pthread_mutex_unlock(&lock);
                return -1;
            }
        }
        n_waiters = 0;
        if ((result = pthread_mutex_unlock(&lock)) != 0) {
            fprintf(stderr, "pthread_mutex_unlock: %s\n", strerror(result));
            return -1;
        }
    } else {
        n_waiters++;
        if ((result = pthread_mutex_unlock(&lock)) != 0) {
            fprintf(stderr, "pthread_mutex_unlock: %s\n", strerror(result));
            return -1;
        }
        if (sem_wait(&semaphore) == -1) {
            perror("sem_wait");
            return -1;
        }
    }

    return 0;
}

int open(const char *pathname, int flags, ...) {
    int (*open_orig)(const char *pathname, int flags);
    open_orig = dlsym(RTLD_NEXT, "open");    // Get pointer to real function
    char *error = dlerror();
    if (error != NULL) {
        fprintf(stderr, "dlsym: %s\n", error);
        return -1;
    }

    // If thread isn't opening a server file, let it proceed
    if (!is_server_file(pathname)) {
        return open_orig(pathname, flags);
    }

    // Otherwise, check in at the barrier
    int barrier_checkin = barrier();
    if (barrier_checkin != 0) {
        return -1;
    }

    return open_orig(pathname, flags);
}

int open64(const char *pathname, int flags, ...) {
    int (*open64_orig)(const char *pathname, int flags);
    open64_orig = dlsym(RTLD_NEXT, "open64");    // Get pointer to real function
    char *error = dlerror();
    if (error != NULL) {
        fprintf(stderr, "dlsym: %s\n", error);
        return -1;
    }

    // If thread isn't opening a server file, let it proceed
    if (!is_server_file(pathname)) {
        return open64_orig(pathname, flags);
    }

    // Otherwise, check in at the barrier
    int barrier_checkin = barrier();
    if (barrier_checkin != 0) {
        return -1;
    }

    return open64_orig(pathname, flags);
}

int openat(int dirfd, const char *pathname, int flags, ...) {
    int (*openat_orig)(int dirfd, const char *pathname, int flags);
    openat_orig = dlsym(RTLD_NEXT, "openat");    // Get pointer to real function
    char *error = dlerror();
    if (error != NULL) {
        fprintf(stderr, "dlsym: %s\n", error);
        return -1;
    }

    // If thread isn't opening a server file, let it proceed
    if (!is_server_file(pathname)) {
        return openat_orig(dirfd, pathname, flags);
    }

    // Otherwise, check in at the barrier
    int barrier_checkin = barrier();
    if (barrier_checkin != 0) {
        return -1;
    }

    return openat_orig(dirfd, pathname, flags);
}

int openat64(int dirfd, const char *pathname, int flags, ...) {
    int (*openat64_orig)(int dirfd, const char *pathname, int flags);
    openat64_orig = dlsym(RTLD_NEXT, "openat64");    // Get pointer to real function
    char *error = dlerror();
    if (error != NULL) {
        fprintf(stderr, "dlsym: %s\n", error);
        return -1;
    }

    // If thread isn't opening a server file, let it proceed
    if (!is_server_file(pathname)) {
        return openat64_orig(dirfd, pathname, flags);
    }

    // Otherwise, check in at the barrier
    int barrier_checkin = barrier();
    if (barrier_checkin != 0) {
        return -1;
    }

    return openat64_orig(dirfd, pathname, flags);
}

int __openat_2(int dirfd, const char *pathname, int flags) {
    int (*__openat2_orig)(int dirfd, const char *pathname, int flags);
    __openat2_orig = dlsym(RTLD_NEXT, "__openat_2");    // Get pointer to real function
    char *error = dlerror();
    if (error != NULL) {
        fprintf(stderr, "dlsym: %s\n", error);
        return -1;
    }

    // If thread isn't opening a server file, let it proceed
    if (!is_server_file(pathname)) {
        return __openat2_orig(dirfd, pathname, flags);
    }

    // Otherwise, check in at the barrier
    int barrier_checkin = barrier();
    if (barrier_checkin != 0) {
        return -1;
    }

    return __openat2_orig(dirfd, pathname, flags);
}

int __openat64_2(int dirfd, const char *pathname, int flags) {
    int (*__openat64_2_orig)(int dirfd, const char *pathname, int flags);
    __openat64_2_orig = dlsym(RTLD_NEXT, "__openat64_2");    // Get pointer to real function
    char *error = dlerror();
    if (error != NULL) {
        fprintf(stderr, "dlsym: %s\n", error);
        return -1;
    }

    // If thread isn't opening a server file, let it proceed
    if (!is_server_file(pathname)) {
        return __openat64_2_orig(dirfd, pathname, flags);
    }

    // Otherwise, check in at the barrier
    int barrier_checkin = barrier();
    if (barrier_checkin != 0) {
        return -1;
    }

    return __openat64_2_orig(dirfd, pathname, flags);
}

FILE *fopen(const char *restrict path, const char *restrict mode) {
    FILE *(*fopen_orig)(const char *restrict path, const char *restrict mode);
    fopen_orig = dlsym(RTLD_NEXT, "fopen");    // Get pointer to real function
    char *error = dlerror();
    if (error != NULL) {
        fprintf(stderr, "dlsym: %s\n", error);
        return NULL;
    }

    // If thread isn't opening a server file, let it proceed
    if (!is_server_file(path)) {
        return fopen_orig(path, mode);
    }

    // Otherwise, check in at the barrier
    int barrier_checkin = barrier();
    if (barrier_checkin != 0) {
        return NULL;
    }

    return fopen_orig(path, mode);
}
