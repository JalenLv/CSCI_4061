#!/bin/bash
# SIGINT while the main thread is blocked in enqueue: 5 slow clients occupy
# the workers, 5 more fill the queue, and an 11th has been accepted. The
# server must not deadlock; all 11 are served.
. test_cases/extra/resources/lib.sh
start_server server_files
pids=()
for i in 1 2 3 4 5; do
    $CLIENT slow "$PORT" /quote.txt server_files/quote.txt 3 > "$WORK/slow$i" &
    pids+=($!)
done
sleep 0.5
for i in 1 2 3 4 5 6; do
    $CLIENT check "$PORT" /index.html server_files/index.html > "$WORK/fast$i" &
    pids+=($!)
done
sleep 1
kill -INT "$SERVER_PID"
wait "${pids[@]}"
cat "$WORK"/slow? "$WORK"/fast?
finish_server
server_output
