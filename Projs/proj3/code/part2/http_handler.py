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
    raise NotImplementedError("TODO: read_request")


# Write an HTTP response to a socket
# 'conn': The socket to write the request to
# 'file_path': A complete path (e.g., "server_files/quote.txt") for the file to
#   send in the response. Send a 404 response if this file does not exist.
def write_response(conn: socket.socket, file_path: str) -> None:
    raise NotImplementedError("TODO: write_response")
