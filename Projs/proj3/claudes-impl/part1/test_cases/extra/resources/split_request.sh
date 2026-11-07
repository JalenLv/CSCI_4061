#!/bin/bash
# The request arrives in many small pieces; the server must keep reading
# until the empty line
. test_cases/extra/resources/lib.sh
start_server server_files
$CLIENT check "$PORT" /quote.txt server_files/quote.txt --chunk 1 --delay 0.005
$CLIENT check "$PORT" /index.html server_files/index.html --chunk 7 --delay 0.05
stop_server
server_output
