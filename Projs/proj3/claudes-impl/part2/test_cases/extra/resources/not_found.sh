#!/bin/bash
# Resources that don't exist get a 404 with Content-Length 0 and no body
. test_cases/extra/resources/lib.sh
start_server server_files
for r in /affordable_gpu.txt /no_such_dir/quote.txt /quote.txt/more /README; do
    $CLIENT check "$PORT" "$r"
done
$CLIENT check "$PORT" /quote.txt server_files/quote.txt
stop_server
server_output
