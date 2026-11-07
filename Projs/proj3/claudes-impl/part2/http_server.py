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


# Serve clients from the queue until it is shut down and empty
def worker(queue: ConnectionQueue, serve_dir: str) -> None:
    while True:
        try:
            conn = queue.dequeue()
        except ShutDownException:
            return
        with conn:  # Closes the connection whatever happens below
            try:
                resource = http_handler.read_request(conn)
                # "/quote.txt" in "server_files" -> "server_files/quote.txt"
                http_handler.write_response(conn, serve_dir + resource)
            except (OSError, ValueError) as e:
                # One client's failure (it disconnected, sent garbage) must not
                # end this thread, or the pool would shrink by one
                print(f"Error serving client: {e}", file=sys.stderr)


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

    signal.signal(signal.SIGINT, handle_sigint)
    try:
        server = socket.create_server(("", port))
    except OSError as e:
        print(f"Failed to start server on port {port}: {e}", file=sys.stderr)
        sys.exit(1)
    # Python runs the SIGINT handler but then restarts an interrupted accept(),
    # and the signal may not even reach the main thread. A timeout makes accept()
    # return every second so the loop can check keep_going.
    server.settimeout(1)

    exit_code = 0
    queue = ConnectionQueue(QUEUE_SIZE)
    workers = []
    try:
        for _ in range(N_WORKERS):
            thread = threading.Thread(target=worker, args=(queue, serve_dir))
            thread.start()
            workers.append(thread)
    except RuntimeError as e:
        print(f"Failed to start worker thread: {e}", file=sys.stderr)
        exit_code = 1

    # Main thread only accepts; a worker talks to the client
    while keep_going and exit_code == 0:
        try:
            conn, _ = server.accept()
        except TimeoutError:
            continue
        except OSError as e:
            print(f"accept failed: {e}", file=sys.stderr)
            exit_code = 1
            break
        # Blocks while the queue is full, capping the number of waiting clients
        try:
            queue.enqueue(conn)
        except ShutDownException:  # Only this thread shuts the queue down
            conn.close()
            exit_code = 1

    # Stop accepting first, then let the workers drain the queue and return
    server.close()
    queue.shut_down()
    for thread in workers:
        thread.join()
    sys.exit(exit_code)


if __name__ == "__main__":
    main()
