#!/bin/bash
# Runs pipe, shmem and threads (3 workers each) on the 2x3 pair under valgrind and prints
# the three products, then three lines that do not depend on the run:
#   - how many processes valgrind checked: the parent plus 3 pipe and 3 shmem children
#     (valgrind follows fork() on its own; threads live inside the parent)
#   - how many of those processes had memory errors (invalid reads/writes, use of
#     uninitialised values); leaks are not counted as errors here
#   - how many leak records, of any kind, were allocated by a call made from
#     hadamard_pipe.c, hadamard_shmem.c or hadamard_threads.c
# CPython itself leaves thousands of blocks allocated at exit, so only records whose
# allocation stack passes through those files count. Every kind counts, not just
# "definitely lost": a child calls exit() from inside hadamard_pipe(), where its
# pointers to A, B and the pid array are still on the stack, so a child that forgets
# to free them shows up as "still reachable".
# Run from part2/. The valgrind logs are kept under test_results/valgrind/.
set -u
here=$(dirname "$0")
mkdir -p test_results/valgrind || exit 1
logs=$(mktemp -d test_results/valgrind/run.XXXXXX) || exit 1

PYTHONMALLOC=malloc valgrind --leak-check=full --show-leak-kinds=all \
    --errors-for-leak-kinds=none --log-file="$logs/vg.%p" \
    python3 "$here/hadamard_call.py" "$here/tiny_2x3_a.bin" "$here/tiny_2x3_b.bin" \
    pipe:3 shmem:3 threads:3

echo "processes checked by valgrind: $(ls "$logs"/vg.* | wc -l)"
echo "processes with memory errors: $(grep -h 'ERROR SUMMARY:' "$logs"/vg.* |
    awk '$4 != 0' | wc -l)"
# A loss record starts with "... in loss record N of M" (valgrind writes numbers above
# 999 with commas) and ends at the next line that holds only the "==pid==" prefix
echo "leak records allocated from hadamard_{pipe,shmem,threads}.c: $(cat "$logs"/vg.* |
    awk '/ in loss record [0-9,]+ of [0-9,]+/ { in_rec = 1; ours = 0; next }
         in_rec && /hadamard_(pipe|shmem|threads)\.c/ { ours = 1 }
         in_rec && /^==[0-9]+== *$/ { n += ours; in_rec = 0 }
         END { print n + 0 }')"
