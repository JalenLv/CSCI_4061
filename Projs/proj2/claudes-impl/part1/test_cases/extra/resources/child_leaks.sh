#!/bin/bash
# Runs swish under valgrind on child_leaks_input.txt, whose pipelines contain
# commands that cannot start: a missing program first, in the middle and last,
# a missing '<' file and an unwritable '>' path. Each of those children exits
# while still running under valgrind, and '%p' gives it its own log, so its
# leaks are visible (testius's own valgrind check only sees the shell's exit
# code). Children that exec successfully stop being traced and write no heap
# summary, so they are not counted. Run from the part1 directory.
logs=test_results/child_leaks
rm -rf "$logs" && mkdir -p "$logs" || exit 1
valgrind --leak-check=full --show-leak-kinds=all --errors-for-leak-kinds=all \
    --log-file="$logs/vg.%p" ./swish < test_cases/extra/resources/child_leaks_input.txt \
    > /dev/null 2>&1

total=0
errors=0
in_use=0
for f in "$logs"/vg.*; do
    grep -q "HEAP SUMMARY" "$f" || continue
    total=$((total + 1))
    grep -q "ERROR SUMMARY: 0 errors" "$f" || errors=$((errors + 1))
    grep -q "in use at exit: 0 bytes in 0 blocks" "$f" || in_use=$((in_use + 1))
done
echo "valgrind reports: $total, with errors: $errors, with memory in use at exit: $in_use"
