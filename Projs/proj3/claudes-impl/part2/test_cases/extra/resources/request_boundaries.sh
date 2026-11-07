#!/bin/bash
# Long requests whose closing "\r\n\r\n" straddles a 512-, 1024-, 4096- or
# 8192-byte read boundary, up to 64 KiB of headers
. test_cases/extra/resources/lib.sh
start_server server_files
for at in 510 511 1022 4094 4095 8190 65534; do
    echo "end of request at byte $at:"
    $CLIENT check "$PORT" /quote.txt server_files/quote.txt --end-at "$at"
done
stop_server
server_output
