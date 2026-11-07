#!/usr/bin/env python3
"""sigpipe_default.py COMMAND [ARGS...]: run COMMAND with SIGPIPE at its default
action (terminate), as when launched from a terminal. testius starts its shell
from Python, which ignores SIGPIPE, and every process it starts inherits that."""
import os
import signal
import sys

signal.signal(signal.SIGPIPE, signal.SIG_DFL)
os.execvp(sys.argv[1], sys.argv[1:])
