#!/bin/bash
# SIGINT while 5 slow clients occupy every worker and 3 more wait in the
# queue. The spec: dequeues continue until the queue is empty, so all 8 are
# served before the server exits.
. test_cases/extra/resources/lib.sh
start_server server_files
pids=()
for i in 1 2 3 4 5; do
    $CLIENT slow "$PORT" /quote.txt server_files/quote.txt 2.5 > "$WORK/slow$i" &
    pids+=($!)
done
sleep 0.5
for f in index.html courses.txt gatsby.txt; do
    $CLIENT check "$PORT" "/$f" "server_files/$f" > "$WORK/queued-$f" &
    pids+=($!)
done
sleep 0.5
kill -INT "$SERVER_PID"
wait "${pids[@]}"
cat "$WORK"/slow1 "$WORK"/slow2 "$WORK"/slow3 "$WORK"/slow4 "$WORK"/slow5
cat "$WORK"/queued-index.html "$WORK"/queued-courses.txt "$WORK"/queued-gatsby.txt
finish_server
server_output
