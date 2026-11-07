#!/bin/bash
# http_server_tsan through concurrent, missing and misbehaving clients, then
# SIGINT: ThreadSanitizer reports no data races
. test_cases/extra/resources/lib.sh
start_server server_files
curl_all "$PORT" "$WORK/dl" 40
$CLIENT check "$PORT" /affordable_gpu.txt
$CLIENT rude "$PORT"
stop_server 10
echo "ThreadSanitizer warnings: $(grep -c 'WARNING: ThreadSanitizer' "$WORK/server.out")"
