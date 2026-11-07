#!/bin/bash
# 40 clients at once: 5 being served, 5 queued, the rest waiting in the
# listen backlog or on the main thread's blocked enqueue
. test_cases/extra/resources/lib.sh
start_server server_files
curl_all "$PORT" "$WORK/dl" 40
stop_server
server_output
