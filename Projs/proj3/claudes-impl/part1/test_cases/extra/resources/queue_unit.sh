#!/bin/bash
# queue_unit.sh [tsan]: build queue_test.c against connection_queue.c (with
# ThreadSanitizer if asked) and run it
WORK=test_results/extra/queue_unit${1:+_$1}
mkdir -p "$WORK"
FLAGS="-Wall -Werror -g -pthread"
[ "$1" = tsan ] && FLAGS="$FLAGS -fno-omit-frame-pointer -fsanitize=thread"
if ! gcc $FLAGS -I. test_cases/extra/resources/queue_test.c connection_queue.c -o "$WORK/queue_test"; then
    echo "build failed"
    exit 1
fi
"$WORK/queue_test" 2> "$WORK/stderr"
echo "exit status: $?"
echo "ThreadSanitizer warnings: $(grep -c 'WARNING: ThreadSanitizer' "$WORK/stderr")"
