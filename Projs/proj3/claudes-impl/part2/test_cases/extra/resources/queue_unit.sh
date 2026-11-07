#!/bin/bash
# Run queue_test.py against connection_queue.py
timeout 60 python3 test_cases/extra/resources/queue_test.py
echo "exit status: $?"
