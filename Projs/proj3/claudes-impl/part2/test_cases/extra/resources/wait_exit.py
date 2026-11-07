#!/usr/bin/env python3
"""wait_exit.py PID SECONDS: wait for process PID (a child of the calling shell)
to exit, without reaping it. Kills it with SIGKILL if it outlives SECONDS."""
import os
import signal
import sys
import time

pid, limit = int(sys.argv[1]), float(sys.argv[2])


def alive():
    try:
        with open(f"/proc/{pid}/stat") as f:
            state = f.read().rsplit(")", 1)[1].split()[0]
    except FileNotFoundError:
        return False
    return state != "Z"  # A zombie has exited; its parent hasn't reaped it yet


deadline = time.monotonic() + limit
while alive() and time.monotonic() < deadline:
    time.sleep(0.02)
if alive():
    print(f"server still running {limit:g} s after SIGINT; killed")
    os.kill(pid, signal.SIGKILL)
else:
    print(f"server exited within {limit:g} s of SIGINT")
