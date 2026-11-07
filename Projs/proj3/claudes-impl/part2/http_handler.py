import pathlib
import socket

MIME_TYPES = {
    ".txt": "text/plain",
    ".html": "text/html",
    ".jpg": "text/jpeg",
    ".png": "image/png",
    ".pdf": "application/pdf",
    ".mp3": "audio/mpeg",
}


# Read in an HTTP request from a socket
# 'conn': The socket to read from
# Returns the name of the requested resource
def read_request(conn: socket.socket) -> str:
    # makefile() buffers the socket so it can be read a line at a time.
    # Closing the file object leaves the socket itself open.
    with conn.makefile("rb") as reader:
        request_line = reader.readline()
        # Consume the headers too, up to the empty line that ends a GET request
        line = request_line
        while line != b"\r\n":
            if line == b"":
                raise ConnectionError("connection closed before the end of the request")
            line = reader.readline()

    # Request line: GET <resource> HTTP/1.x
    parts = request_line.decode("utf-8").split()
    if len(parts) < 2 or parts[0] != "GET" or not parts[1].startswith("/"):
        raise ValueError(f"malformed request line {request_line!r}")
    return parts[1]


# Write an HTTP response to a socket
# 'conn': The socket to write the request to
# 'file_path': A complete path (e.g., "server_files/quote.txt") for the file to
#   send in the response. Send a 404 response if this file does not exist.
def write_response(conn: socket.socket, file_path: str) -> None:
    path = pathlib.Path(file_path)
    if not path.is_file():  # Missing, or something like a directory
        conn.sendall(b"HTTP/1.0 404 Not Found\r\nContent-Length: 0\r\n\r\n")
        return

    content_type = MIME_TYPES.get(path.suffix, "application/octet-stream")
    # Open before sending anything, so a failure here doesn't leave the client
    # with a 200 status and no body
    with path.open("rb") as file:
        header = (
            "HTTP/1.0 200 OK\r\n"
            f"Content-Type: {content_type}\r\n"
            f"Content-Length: {path.stat().st_size}\r\n"
            "\r\n"
        )
        conn.sendall(header.encode("utf-8"))
        conn.sendfile(file)  # The file exactly as stored, sent in chunks
