#!/bin/bash
# A second server on a taken port fails promptly with a nonzero status; the
# first keeps working
. test_cases/extra/resources/lib.sh
start_server server_files
timeout 5 $SERVER server_files "$PORT" > "$WORK/second.out" 2>&1
status=$?
if [ $status -eq 124 ]; then
    echo "second server still running after 5 s"
elif [ $status -eq 0 ]; then
    echo "second server exited with status 0"
else
    echo "second server exited with a nonzero status"
fi
if grep -q "Address already in use" "$WORK/second.out"; then
    echo "its output mentions 'Address already in use'"
else
    echo "its output doesn't mention 'Address already in use'"
fi
$CLIENT check "$PORT" /quote.txt server_files/quote.txt
stop_server
server_output
