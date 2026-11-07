#!/bin/bash
# Status line, Content-Type, Content-Length and body for every file
. test_cases/extra/resources/lib.sh
start_server server_files
for f in $FILES; do
    $CLIENT check "$PORT" "/$f" "server_files/$f"
done
stop_server
server_output
