import signal
import socket
import sys
import threading

import http_handler
from connection_queue import ConnectionQueue, ShutDownException

QUEUE_SIZE = 5
N_WORKERS = 5
keep_going = True


def handle_sigint(_signo, _frame):
    global keep_going
    keep_going = False


def main():
    if len(sys.argv) < 3:
        print(f"Usage: {sys.argv[0]} <directory> <port>")
        sys.exit(1)
    serve_dir = sys.argv[1]
    try:
        port = int(sys.argv[2])
    except ValueError:
        print(f"Invalid port number: {sys.argv[2]}")
        sys.exit(1)

    # TODO Complete the implementation of the rest of this function


if __name__ == "__main__":
    main()
