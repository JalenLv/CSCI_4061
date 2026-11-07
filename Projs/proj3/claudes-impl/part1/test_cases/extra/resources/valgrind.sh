#!/bin/bash
# The server under valgrind (testius's options) through concurrent, missing
# and misbehaving clients, then SIGINT: no errors, nothing left allocated
. test_cases/extra/resources/lib.sh
start_server server_files valgrind --leak-check=full --show-leak-kinds=all \
    --errors-for-leak-kinds=all --track-origins=yes --log-file="$WORK/valgrind.log"
curl_all "$PORT" "$WORK/dl" 15
$CLIENT check "$PORT" /affordable_gpu.txt
$CLIENT rude "$PORT"
stop_server 20
grep -o "ERROR SUMMARY: [0-9]* errors" "$WORK/valgrind.log"
grep -o "All heap blocks were freed -- no leaks are possible\|in use at exit: .*" "$WORK/valgrind.log"
