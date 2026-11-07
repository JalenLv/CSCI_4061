#!/bin/bash
# Like provided test 2, but 25 requests: five rounds through the
# concurrent_open barrier, each needing all 5 workers at once
. test_cases/extra/resources/lib.sh
start_server server_files env LD_PRELOAD=./concurrent_open.so
curl_all "$PORT" "$WORK/dl" 25
stop_server
server_output
