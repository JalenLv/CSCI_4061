#!/bin/bash
# SIGINT with no clients at all
. test_cases/extra/resources/lib.sh
start_server server_files
sleep 0.3
stop_server 3
server_output
