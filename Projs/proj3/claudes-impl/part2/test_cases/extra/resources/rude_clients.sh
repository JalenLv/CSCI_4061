#!/bin/bash
# Clients that disconnect without a full request, or send a malformed one.
# The server may report them, but must keep serving and still exit cleanly.
. test_cases/extra/resources/lib.sh
start_server server_files
$CLIENT rude "$PORT"
$CLIENT check "$PORT" /quote.txt server_files/quote.txt
stop_server
