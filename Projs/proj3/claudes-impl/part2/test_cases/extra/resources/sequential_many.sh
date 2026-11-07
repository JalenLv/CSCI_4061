#!/bin/bash
# 1500 requests, more than the 1024-descriptor limit: a descriptor leaked per
# request (client socket or file) runs out partway through
. test_cases/extra/resources/lib.sh
start_server server_files
before=$($CLIENT fds "$SERVER_PID")
$CLIENT many "$PORT" server_files 1500
after=$($CLIENT fds "$SERVER_PID")
echo "open descriptors after the requests: $((after - before)) more than before"
stop_server
server_output
