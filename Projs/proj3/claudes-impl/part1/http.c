#include "http.h"

#include <assert.h>
#include <errno.h>
#include <fcntl.h>
#include <stdio.h>
#include <string.h>
#include <sys/stat.h>
#include <unistd.h>

#define BUFSIZE 512

const char *get_mime_type(const char *file_extension) {
    if (strcmp(".txt", file_extension) == 0) {
        return "text/plain";
    } else if (strcmp(".html", file_extension) == 0) {
        return "text/html";
    } else if (strcmp(".jpg", file_extension) == 0) {
        return "image/jpeg";
    } else if (strcmp(".png", file_extension) == 0) {
        return "image/png";
    } else if (strcmp(".pdf", file_extension) == 0) {
        return "application/pdf";
    } else if (strcmp(".mp3", file_extension) == 0) {
        return "audio/mpeg";
    }

    return NULL;
}

// Reads the request until its terminating empty line, however the client
// splits it into packets. The request line is kept (it must be shorter than
// BUFSIZE, so the resource name copied out of it is too); the headers are read
// and discarded.
int read_http_request(int fd, char *resource_name) {
    const char *end_marker = "\r\n\r\n";
    char buf[BUFSIZE];
    char line[BUFSIZE];
    int line_len = 0;
    int line_done = 0;
    int matched = 0;    // How many bytes of end_marker the request read so far ends with

    while (matched < 4) {
        ssize_t nbytes = read(fd, buf, BUFSIZE);
        if (nbytes == -1) {
            perror("read");
            return -1;
        } else if (nbytes == 0) {
            fprintf(stderr, "read_http_request: connection closed before the end of the request\n");
            return -1;
        }

        for (ssize_t i = 0; i < nbytes && matched < 4; i++) {
            char c = buf[i];
            if (!line_done) {
                if (c == '\n') {
                    line_done = 1;
                } else if (line_len == BUFSIZE - 1) {
                    fprintf(stderr, "read_http_request: request line too long\n");
                    return -1;
                } else {
                    line[line_len++] = c;
                }
            }
            // The marker can straddle two reads, so match it byte by byte
            if (c == end_marker[matched]) {
                matched++;
            } else {
                matched = (c == '\r') ? 1 : 0;
            }
        }
    }

    if (line_len > 0 && line[line_len - 1] == '\r') {
        line_len--;
    }
    line[line_len] = '\0';

    // Request line: GET <resource> HTTP/1.x
    char *saveptr;
    char *method = strtok_r(line, " ", &saveptr);
    char *resource = strtok_r(NULL, " ", &saveptr);
    if (method == NULL || resource == NULL || strcmp(method, "GET") != 0 || resource[0] != '/') {
        fprintf(stderr, "read_http_request: malformed request line\n");
        return -1;
    }
    strcpy(resource_name, resource);
    return 0;
}

// write() may write less than asked; keep going until everything is sent
static int write_all(int fd, const char *buf, size_t len) {
    while (len > 0) {
        ssize_t nbytes = write(fd, buf, len);
        if (nbytes == -1) {
            perror("write");
            return -1;
        }
        buf += nbytes;
        len -= nbytes;
    }
    return 0;
}

static int write_not_found(int fd) {
    const char *response = "HTTP/1.0 404 Not Found\r\n"
                           "Content-Length: 0\r\n"
                           "\r\n";
    return write_all(fd, response, strlen(response));
}

int write_http_response(int fd, const char *resource_path) {
    struct stat file_info;
    if (stat(resource_path, &file_info) == -1) {
        // ENOTDIR: a path like quote.txt/x, which can't exist either
        if (errno != ENOENT && errno != ENOTDIR) {
            perror("stat");
            return -1;
        }
        return write_not_found(fd);
    }
    if (!S_ISREG(file_info.st_mode)) {
        return write_not_found(fd);    // A directory, for instance: nothing to send
    }

    // Open before sending anything, so a failure here doesn't leave the client
    // with a 200 status and no body
    int file_fd = open(resource_path, O_RDONLY);
    if (file_fd == -1) {
        perror("open");
        return -1;
    }

    // The extension is whatever follows the last '.' of the file name
    const char *file_name = strrchr(resource_path, '/');
    file_name = (file_name == NULL) ? resource_path : file_name + 1;
    const char *extension = strrchr(file_name, '.');
    const char *mime_type = (extension == NULL) ? NULL : get_mime_type(extension);
    if (mime_type == NULL) {
        mime_type = "application/octet-stream";    // Generic binary data
    }

    char header[BUFSIZE];
    int header_len = snprintf(header, sizeof(header),
                              "HTTP/1.0 200 OK\r\n"
                              "Content-Type: %s\r\n"
                              "Content-Length: %lld\r\n"
                              "\r\n",
                              mime_type, (long long) file_info.st_size);
    if (header_len < 0 || (size_t) header_len >= sizeof(header)) {
        fprintf(stderr, "write_http_response: response header too long\n");
        close(file_fd);
        return -1;
    }
    if (write_all(fd, header, header_len) == -1) {
        close(file_fd);
        return -1;
    }

    // Body: the file exactly as stored, one chunk at a time
    char buf[BUFSIZE];
    ssize_t nbytes;
    while ((nbytes = read(file_fd, buf, BUFSIZE)) > 0) {
        if (write_all(fd, buf, nbytes) == -1) {
            close(file_fd);
            return -1;
        }
    }
    if (nbytes == -1) {
        perror("read");
        close(file_fd);
        return -1;
    }
    if (close(file_fd) == -1) {
        perror("close");
        return -1;
    }
    return 0;
}
