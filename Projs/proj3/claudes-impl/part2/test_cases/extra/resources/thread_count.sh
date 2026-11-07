#!/bin/bash
# A fixed pool: main thread plus 5 workers, before and after serving clients
. test_cases/extra/resources/lib.sh
start_server server_files
echo "threads before any client: $($CLIENT threads "$SERVER_PID")"
$CLIENT many "$PORT" server_files 100
echo "threads after 100 sequential requests: $($CLIENT threads "$SERVER_PID")"
curl_all "$PORT" "$WORK/dl" 20
echo "threads after 20 concurrent requests: $($CLIENT threads "$SERVER_PID")"
stop_server
server_output
