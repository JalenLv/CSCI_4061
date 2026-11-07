# CSCI 4061 -- Project 3: Multi-Threaded HTTP Server

> Archived from the Canvas assignment page
> <https://canvas.umn.edu/courses/579183/assignments/5461477>
>
> | | |
> |---|---|
> | **Due** | Fri Nov 20, 2026 11:59pm |
> | **Points** | 100 |
> | **Available** | Nov 2, 2026 12:00am - Nov 20, 2026 11:59pm |

---

**Due Dates**

- [Project Quiz](https://canvas.umn.edu/courses/579183/quizzes/1209695): 11:59pm, Friday 11/06/26 (Canvas)
- Final Submission: 11:59pm, Friday 11/20/26 (Gradescope)

6.25% of Course Grade

Projects are to be completed **individually or with a partner. Collaboration outside of your project team is prohibited.** See our [syllabus](https://canvas.umn.edu/courses/579183/assignments/syllabus) for full academic integrity policies.

**Starter Code:** [proj3-code.zip](https://csci4061-fa26.s3.amazonaws.com/proj3-code.zip)

Download and unpack the starter code linked above. Place the unpacked `proj3-code` *inside* of your existing `csci4061-fa26` directory to ensure you can access the project within your Docker environment.

## Introduction

HTTP (and its encrypted counterpart, HTTPS) is *the* protocol powering the World Wide Web. Your browser acts as an HTTP client, while all websites and their contents are made accessible by processes acting as HTTP servers. You will gain firsthand experience with HTTP and the underlying TCP sockets over which HTTP content is communicated in this project.

In Part 1 of the project, you will implement an HTTP server in C. You will need to perform the necessary socket setup, parse incoming client HTTP requests, and then reply with valid HTTP responses that communicate the contents of files stored locally on disk.

Initially, your server will interact with clients sequentially, meaning there can be only one active client session at any particular time. You will convert your single-threaded server into a multi-threaded server featuring a design that more closely resembles the real servers used to power the modern web. In particular, we will use the *thread pool* paradigm to create a fixed-size collection of threads, each capable of interacting with one client.

Your server will be able to interact with any HTTP client, such as the HTTP client implemented in lab 9, the `curl` and `wget` command-line utilities, or a web browser.

In Part 2 of the project, you will reimplement your multi-threaded web server in Python. You will likely find this much simpler than the C implementation, although you will also see that you have less influence over implementation details like memory management and error handling. This illustrates the classic tradeoff between abstraction/convenience and control/performance inherent in moving from C to a higher-level programming language.

This project will focus on a few important systems programming topics:

- TCP server socket setup and initialization
- Server-side TCP communication with `accept()` followed by socket and file I/O
- HTTP request parsing and response generation
- Clean server termination through signal handling
- Thread creation and management
- Implementation of a thread-safe queue data structure

If you are working with a partner, we recommend that you **do not** assign Part 1 to one partner and Part 2 to the other partner. Part 1 will, most likely, be significantly more difficult and take more time than Part 2.

## Grading Criteria

- **Project Quiz (10%):** We have posted [a quiz](https://canvas.umn.edu/courses/579183/quizzes/1209695) about this assignment description on our class Canvas site. Complete the quiz by the due date. The quiz is to be completed **individually** but multiple attempts are allowed.
- **Automated Testing (40%):** We have provided automated tests along with instructions on how to run these tests from the command line. To pass the tests, you will need to ensure that your code compiles and runs according to the specifications given. Note that we have some additional tests we will run against your code that are not provided to you.
- **Oral Exam (40%):** Shortly after the project due date, you will sign up for a brief oral exam with the TAs, who will ask you questions about your submitted code and the extent to which you understand your work. Oral exams will be done individually, not together with your project partner if you choose to work with one.
- **Error Checking (10%):** TAs will manually review your code to evaluate the extent to which your code thoroughly and appropriately checks for and handles error conditions. **Make sure that you properly check for and deal with errors whenever using a C or Python function that may indicate an error.**

## Makefile

There are three Makefiles provided as part of this project: one in the top-level `proj3-code` directory, a second in the `part1` subdirectory, and a third in the `part2` subdirectory.

The top-level Makefile supports the following commands:

- `make`: Compile all code, produce executable program(s)
- `make clean`: Remove all compiled items from `part1` and `part2`. Useful if you want to recompile everything from scratch.
- `make clean-tests`: Remove all files produced during the execution of the tests in `part1` and `part2`.
- `make test`: Run all test cases
- `make zip`: Create a zip file for submission to Gradescope

The Makefiles in the `part1` and `part2` subdirectories support the following commands:

- `make`: Compile all code, produce executable program(s) for this part of the project
- `make clean`: Remove all compiled items from this part of the project. Useful if you want to recompile everything from scratch.
- `make clean-tests`: Remove all files produced during the execution of the tests for this part of the project.
- `make test`: Run all test cases for this part of the project `make test testnum=5`: Run test case \#5 only.

## Starter Code

### Part 1

| **File**             | **Purpose** | **Notes**                                          |
|----------------------|-------------|----------------------------------------------------|
| `Makefile`           | Build       | Build file to compile and test code                |
| `http.h`             | Provided    | Header file for HTTP utility functions             |
| `http.c`             | *EDIT*      | Implementation of HTTP utility functions           |
| `http_server.c`      | *EDIT*      | Main HTTP Server Program                           |
| `connection_queue.h` | *EDIT*      | Header file for thread-safe queue data structure   |
| `connection_queue.c` | *EDIT*      | Implementation of thread-safe queue data structure |
| `concurrent_open.c`  | Testing     | Supports testing of multithreaded server           |
| `server_files/`      | Testing     | Sample files for your server to offer to clients   |
| `test_cases/`        | Testing     | Test cases and related materials                   |

Starter Code Files for Part 1

### Part 2

| **File**              | **Purpose** | **Notes**                                          |
|-----------------------|-------------|----------------------------------------------------|
| `Makefile`            | Build       | Build file to compile and test code                |
| `http_handler.py`     | *EDIT*      | Implementation of HTTP utility functions           |
| `http_server.py`      | *EDIT*      | Main HTTP Server Program                           |
| `connection_queue.py` | *EDIT*      | Implementation of thread-safe queue data structure |
| `concurrent_open.c`   | Testing     | Supports testing of multithreaded server           |
| `server_files/`       | Testing     | Sample files for your server to offer to clients   |
| `test_cases/`         | Testing     | Test cases and related materials                   |

Starter Code Files for Part 2

**Note that you are only allowed to change the files marked as *EDIT* above. For all other files, the Gradescope autograder will ignore any modifications you make and use the standard version provided with the starter code.**

## Part 1

In this part of the project, you will implement an HTTP server in C. You should first implement and test a sequential server, then extend your implementation to support multiple clients with a pool of worker threads.

This will involve setting up a TCP server socket and accepting incoming client connection requests in the `main()` function defined in the `http_server.c` file. Then, you'll need to implement HTTP request parsing and response generation by completing the utility functions defined in `http.c`. Once your sequential server is working, you'll need to complete the code in `connection_queue.h` and `connection_queue.c` and adapt your code in `http_server.c` to use this queue for communication between multiple threads.

Note that the `http_server` program accepts two command-line arguments:

1.  The directory containing all files to be made available to HTTP clients. For example, if this is `server_files` and a client requests the resource `/quote.txt`, then your server will construct an HTTP response with the contents of `server_files/quote.txt`.
2.  The port for the server to bind to, e.g., `8000`.

### Task 1: Sequential Server

**You must complete your implementation using the provided structure. You are free to write your own additional helper functions but you must implement and use the functions we describe below.**

### Completing http_server.c

#### Socket Setup

First, you will need to establish a TCP server socket that will be able to initiate new connections with TCP clients. This involves the usual sequence of system calls.

1.  `getaddrinfo()` to assemble all of the necessary information for future system calls. Don't forget that we want a TCP socket and will be acting as a server rather than a client.
2.  `socket()` to create a new socket file descriptor
3.  `bind()` to reserve a specific port (given as a command-line argument) for the server process
4.  `listen()` to designate our socket file descriptor as a server socket

#### Main Server Loop

Use the `accept()` system call in a loop to generate a new connection socket for each client that connects to your server.

Your loop will need to perform the following steps:

1.  Call the `read_http_request()` function (which you will also need to implement, details below) to read the client's HTTP request from the connection socket and extract the name of the requested resource (e.g., `/quote.txt`).
2.  Convert the requested resource name to a proper file path. For example, if the client requested the resource `/quote.txt` and the server's resource directory (specified as a command-line argument) is `server_files`, then the proper path to the requested resource on the local file system would be `server_files/quote.txt`.
3.  Call `write_http_response()` (which you will also need to implement, details below), providing the full path to the resource as an argument.
4.  When done, close the connection socket.

#### Signal Handling for Proper Cleanup

In theory, we expect a server to potentially run forever, calling `accept()` in an endless loop. However, we need a way to cleanly shut down our server on demand. We can accomplish this by installing a handler for the `SIGINT` signal. When the server receives this signal, it should break out of its loop and perform the necessary cleanup steps before exiting.

We have already given you a global variable and a handler function to use for `SIGINT`. Your job is to call `sigaction` at the beginning of `main()` to install this handler, structure your `accept()` loop so that it will end after `SIGINT` is received, and add code after this loop to clean things up before the process exits. In particular, you will want to close your server socket file descriptor.

#### Port Reservation Issues

Only one process on a particular computer may bind to a port at a time. You may see an error message like `address already in use` (assuming you call a function like `perror()` in your code) if you call `bind()` for a port that is already taken.

When this happens, simply try a different port number. You should be free to use any number between `1025` and `65535`. Attempting to bind to a port between `0` and `1024` will produce a `permission denied` error.

There is one further complication. When a server process exists, its bound port is not immediately free for reuse. The operating system maintains a timeout called the *linger period* after a bound server socket is closed. The socket's bound port may not be used for a new socket until this timeout expires. A typical linger period is approximately two minutes.

This means that, if you terminate your server process and then try to quickly re-run it with the same port, you may see an `address in use` error as described above. You will need to use a different port until the OS timeout for the previous port expires.

#### Hints

- The socket lectures have accompanying TCP server sample code that you will probably find helpful.
- Remember, `read_http_request()` extracts a resource name (like `/quote.txt`) but `write_http_request()` expects a full *resource path* that is understandable by your local file system such as `server_files/quote.txt`.
- You can refer back to the signal handling lab as a reference and take a very similar approach here.

This is one of the few cases where we **do not** want to use the `SA_RESTART` flag for our signal handler. Most likely, `SIGINT` will arrive when your server is blocked in a call to `accept()`, and we want to break out of the current loop rather than calling `accept()` again.

If `accept()` was interrupted by a signal, it will return `-1` and set `errno` to `EINTR`. You can use this to determine if `accept()` actually failed or if we are trying to shut down the server and should break out of the current loop to perform cleanup code.

### Completing http.c

You will need to implement two functions to handle HTTP communications. These functions are defined in `http.c` and called in `http_server.c`. In this project, we are using HTTP version `1.0`.

#### Reading HTTP Requests

Any valid HTTP request consists of the following elements:

1.  A *request line* containing the operation to perform (method), a resource name, and a protocol version.
2.  A sequence of *request headers* (i.e., key/value pairs), each on their own line
3.  An empty line
4.  Optionally, a request body

In this project, we will consider only the HTTP `GET` operation. HTTP GET requests do not have a body, so all requests will end with an empty line. You are also free to ignore any request headers for this project - our simple server won't need them.

Note that, in HTTP, line endings are denoted by the character sequence `\r\n` rather than the `\n` we are more accustomed to.

For example, a valid HTTP request for the resource `/quote.txt` would be as follows:

``` text
GET /quote.txt HTTP/1.0\r\n
Accept: text/plain\r\n
User-Agent: Chrome/100.4.81516\r\n
\r\n
```

The `read_http_request()` should `read()` an HTTP request (basically, just a long string) from the specified socket file descriptor and return a string containing only the name of the requested resource. In the example request above, that resource name is `/quote.txt`.

Note that you should still use `read()` to consume the entirety of the client's submitted request. You are free to ignore unneeded information like request headers but you should still consume them.

#### Hints

The end of an HTTP request with no body (which is the case for all requests you'll need to deal with in this project) is marked by an empty line, i.e., you will see the characters `\r\n` to end the penultimate line followed by another `\r\n` to express the last, empty line.

You may find some of the techniques demonstrated in the `read_http_response()` function of the HTTP lab code helpful here. However, **you should be sure you understand the relationship between the `stdio` library calls and the regular I/O system calls as well as how `stdio`'s buffering really works if you want to take this approach. If you aren't careful, you could encounter time-consuming and confusing I/O bugs.** If in doubt, just use regular `read()` system calls.

Be careful if you decide to use `strtok()` here. `strtok()` will modify the string being tokenized, and subsequent calls to `strtok()` will modify the strings returned from previous calls. Additionally, `strtok()` is not safe to be used from multiple threads. You can use the re-entrant variant of this function instead.

#### Writing HTTP Responses

Any valid HTTP response consists of the following elements:

1.  A *status line* giving the protocol version and outcome of the client's request
2.  A sequence of *response headers* (i.e., key/value pairs), each on their own line
3.  An empty line
4.  A response body

Remember, line endings in HTTP use `\r\n` rather than `\n` as we are used to.

For the `GET` requests we are considering in this project, the response body is simply the contents of the requested resource (a file stored on the server's file system).

There are only two possible status values your server needs to return for this project:

1.  `200 OK`: The requested resource exists and its contents are included in the response.
2.  `404 Not Found`: The requested resource does not exist. The response contains no body.

For example, if the client were to request the non-existent resource `/affordable_gpu.txt`, then a valid HTTP response is the following:

``` text
HTTP/1.0 404 Not Found\r\n
Content-Length: 0\r\n
\r\n
```

Instead, say the client requests the resource `/quote.txt`, the server's resource directory is `server_files`, and the contents of `server_files/quote.txt` are as follows (with newlines represented explicitly for clarity):

``` text
Premature optimization is the root of all evil.\n
    -- Donald Knuth\n
```

A valid HTTP response in this situation is the following (again with line endings represented explicitly for clarity):

``` text
HTTP/1.0 200 OK\r\n
Content-Type: text/plain\r\n
Content-Length: 68\r\n
\r\n
Premature Optimization is the root of all evil.\n
    -- Donald Knuth\n
```

Note that HTTP's rules for `\r\n` line endings only apply to the status line, response headers, and empty line separating the last header and start of the response body. The response body should contain the file's contents *exactly as they are stored on disk*.

Your HTTP responses **must always contain two response headers for `200 OK` responses:**

1.  `Content-Length`: The number of bytes in the response body. Note that this does *not* include the bytes used to represent the status line or the response headers that precede the body.
2.  `Content-Type`: A string indicating the type of data in the response body. There are a standard set of *MIME Type* strings for each file type. We have provided a helper function in `http.c` that will convert file extensions (e.g., `.txt`) to the corresponding MIME type (e.g., `text/plain`) for you.

A `404 Not Found` response should contain the `Content-Length` header with a value of `0`.

Omitting or using incorrect values for these response headers will lead to incorrect server behavior.

#### Hints

- The `stat()` system call is very helpful here. You can use it both to check if a file exists (if it does not, `stat()` returns `-1` and sets `errno` to `ENOENT`) and to get the number of bytes in the file (which you'll need for the `Content-Length` header)
- The `Content-Length` header is important - most HTTP clients (including web browsers) need it to make sense of the data being sent to them.
- **Make sure you are reading from the file and writing to the TCP socket in chunks, not all at once.** A good chunk size is 512 or 4096 bytes.

### Testing Your Server

First, you will need to run your server process. For example, to run your server on port `4061` and to make the files in the directory `server_files` available to clients, you would execute the command

``` console
> ./http_server server_files 4061
```

While your server is still up and running, you can then use an HTTP client (either in a separate shell session or from a separate program like a web browser) to test the server's functionality. You have access to several possible clients. One possible option is your HTTP client from lab. This has the advantage of being easily modified (you have the source code) if you'd like to print out more information for debugging.

There are also several command-line tools you can use to submit an HTTP request to your server. Remember, you can use `127.0.0.1` or `localhost` to refer to the IP address of your local machine.

Most web tools use `:` to separate address and port, so we often use syntax like `localhost:4061` to try to contact a server process running on the current computer and bound to port `4061`.

To print the contents of a response directly to the screen, use `curl`. For example with a sever bound to port `8000` and to request the resource `/quote.txt`:

``` console
> curl localhost:8000/quote.txt
Premature optimization is the root of all evil.
    -- Donald Knuth
```

To save the body of an HTTP response to a file, use `wget`. For example with a server bound to port `8000` and to request the resource `/quote.txt`:

``` console
> wget localhost:8000/quote.txt
--2026-10-19 14:27:11--  http://localhost:8000/quote.txt
Resolving localhost (localhost)... 127.0.0.1
Connecting to localhost (localhost)|127.0.0.1|:8000... connected.
HTTP request sent, awaiting response... 200 OK
Length: 68 [text/plain]
Saving to: 'quote.txt'

      quote.txt                                                   100% 6.43M=0s

2026-10-19 14:27:11 (19.0 MB/s) - 'quote.txt' saved [68/68]
> cat quote.txt
Premature optimization is the root of all evil.
    -- Donald Knuth
```

By far the most satisfying way to verify that your HTTP server is working correctly is to access a cat picture with your web browser of choice. For example:

![A web browse displaying a picture of an ocelot](ocelot_server.jpg)

By the end of this task, your code should pass **the first** of the two provided test cases. Your code will pass the second test case once you've completed the next task.

#### Tests and Ports

By default, the `make test` target will start your `http_server` program and instruct it to bind to port `8000` for the first test, and then port `8001` for the second test.

This can lead to errors from the `bind()` system call if some other process has claimed those same ports (or a process using one of the ports has recently terminated and the linger period has not yet expired).

You should adjust the port used by your server as needed to avoid collisions with other processes. The `test` target accepts an optional `port_base` argument to change the range of port numbers used by your `http_server` program for the test cases.

For example, to run `make test` and have your server bind to port `4061` and then `4062` for the two tests, you can use the following command:

``` console
> make test port_base=4061
```

### Task 2: Multi-Threaded Server

Next, you will modify your single-threaded server to a multi-threaded implementation. This gives it the ability to concurrently communicate with multiple clients, something any real web server would need to do.

Threads help avoid the high overhead of forking several child processes. In fact, to avoid the costs of repeatedly creating and destroying threads as clients connect to and disconnect from the server, we will use a *thread pool* design pattern. The main thread of our server handles accepting new client connections with `accept()` and then passes responsibility for communicating with the new client off to a "worker" thread in the thread pool.

You will *not* need to make any changes to `http.c` in this task.

### Thread-Safe Queue Data Structure

The key to this design is the implementation of a queue data structure to pass work from the main thread to the thread pool and to coordinate the activities of each thread in the pool. The queue will store file descriptors for the client-specific connection sockets that are generated by calls to `accept()`.

This queue will also need to be *thread safe*, meaning that even with multiple threads concurrently accessing the memory locations storing the queue's data, the queue's state and its elements do not become corrupted. Furthermore, the queue will have some helpful timing synchronization built in. When a thread tries to add to a full queue, it is blocked until space is made available. When a thread tries to remove from an empty queue, it is blocked until a data element is made available.

Take a look at the `connection_queue.h` file. It details the interface for the thread-safe queue data structure that your server will be relying on. You are free to modify the `connection_queue_t` type definition to add fields to the `struct` definition.

**However, you are not allowed to modify any of the function declarations in this file. You must implement the given functions exactly as they are provided to you.**

**Your queue must also have a fixed maximum capacity. We have defined the `CAPACITY` macro in `connection_queue.h` for this purpose.**

#### connection_queue_init()

This function creates and initializes a new `connection_queue_t` instance. This is where you will need to allocate any necessary memory and/or initialize any synchronization primitives like mutexes and condition variables.

#### connection_queue_enqueue()

This function adds a new file descriptor to the queue. If the queue is already full (it already contains the maximum number of possible elements when this function is called), then this function blocks the calling thread until space becomes available, at which point the new element is added to the queue. If this operation adds a new element to what was an empty queue, one thread waiting to dequeue is unblocked.

#### connection_queue_dequeue()

This function removes a file descriptor from the queue and returns it. If the queue is empty (it does not contain any elements when this function is called), then this function blocks the calling thread until a new element is added to the queue. If this operation frees up a slot in what was a full queue, one thread waiting to enqueue is unblocked.

#### connection_queue_shutdown()

This function is called to shut down the queue. Any threads that are blocked in an enqueue operation are unblocked and receive a return value indicating an unsuccessful enqueue. Threads can continue to perform a dequeue operation until the queue becomes empty, at which point they receive a return value indicating an unsuccessful dequeue. This will be essential to cleanly shutting down the server, described in more detail below.

#### connection_queue_free()

This function is called after the queue has been shut down. Any allocated memory or synchronization primitives associated with this queue instance are cleaned up by this function.

#### Hints

- Refer back to the lab on thread-safe data structures for some relevant example code that will help you complete this task.
- Remember, you should always hold a mutex before you signal a condition variable.
- It's fairly easy to write code where your threads get "stuck" forever. Double check that you are signaling condition variables and unlocking mutexes where needed.
- For the shutdown operation, you will find `pthread_cond_broadcast()` useful.

### Modifying the Server Implementation

There are a few, specific changes you will need to make to your HTTP server.

First, you will need to initialize a `connection_queue_t` instance at the beginning of `main()`.

Before entering the main `accept()` loop, you should create a fixed number of "worker" threads, which together make up your server's thread pool. We have defined the `N_THREADS` macro at the top of `http_server.c` giving the number of threads that should be in the pool.

Each thread needs to perform the following tasks in a loop:

1.  Dequeue a client connection socket file descriptor from the connection queue. This will block the thread if the queue is empty.
2.  Use your existing HTTP utility functions to read an HTTP request and write an HTTP response back over the connection socket.

After the thread pool is created, the main thread enters the `accept()` loop. Now that the server has a worker pool of threads, the main thread does not communicate at all with each new client after `accept()` returns. Instead, it adds a file descriptor for the client's connection socket to the connection queue, which will allow one of the worker threads to deal with the client.

Note that the main thread may block if it tries to add to a full queue. This is intended; it is a natural way of putting a cap on the number of concurrently active clients.

#### Cleanly Shutting Down the Server

This is one of the trickiest aspects of the multi-threaded server implementation. We still want to be able to cleanly exit the server when the process receives `SIGINT`. However, signal handling in multi-threaded processes can be difficult.

When a process receives a signal, that signal may be delivered to *any* thread in the process that does not have the signal blocked. To keep things simple, you should configure all of your worker threads to block all possible signals -- forcing delivery to occur only in the main thread.

A new thread inherits the signal mask of the thread that created it (similar to how child processes inherit the parent's signal mask with a call to `fork()`). Therefore, you can do the following when setting up the server's thread pool:

1.  Use the `sigprocmask()` system call to block all possible signals *before* creating any threads. You should also save the current process signal mask (you finally get to use that third argument!).
2.  Create all worker threads for the thread pool -- they will now block all signals as well because they inherit a full signal mask.
3.  Restore the old signal mask from before Step 1, allowing the main thread to once again take delivery of signals.

A `SIGINT` should cause the server to break out of its `accept()` loop just as it did in Task 1. Now, your cleanup code involves a few additional steps:

1.  Use `connection_queue_shutdown()` to allow all worker threads to unblock and cleanly exit.
2.  Use `pthread_join()` to clean up all worker threads.

### Testing Your Server

Once you have the multi-threaded server correctly implemented, your server program should pass **both** of the provided test cases. The second test specifically checks for your server's ability to handle multiple client connections by concurrently requesting several files from the server.

This test will not pass when run against a single-threaded HTTP server. Instead, it specifically checks that your server can maintain `n` active client connections, where `n` is the number of worker threads in the pool.

We use a trick to intercept invocations of the `open()` system call made by the worker threads in your server. Our setup forces each thread to block in its call to `open()` until there are several pending calls to `open()` from multiple threads -- demonstrating that your server is actually handling client requests in concurrently executing threads.

Once enough pending calls to `open()` have accumulated, all are allowed to proceed, meaning your worker threads are allowed to complete their respective tasks.

**One of the most common reasons for failing test 2 is a timeout error, which means the test was not able to complete within the ten-second time limit. There are two main reasons for this:**

1.  Your server does not properly set up the expected number of worker threads, meaning it cannot actually handle the required number of concurrent client requests.
2.  Your server is not able to shut down properly upon receiving `SIGINT` and becomes stuck.

Note that your tests may also time out if your threads become "stuck" for other reasons as well. This could include failing to unlock a mutex from one thread so that other threads can proceed, failing to signal a condition variable to wake up another waiting thread, or failing to properly broadcast to a condition variable when shutting down the connection queue.

If you would like to run your server using the same `open()` intercepting method used by test 2, use the following command structure:

``` console
> LD_PRELOAD=./concurrent_open.so ./http_server <serve_dir> <port>
```

Don't forget about issues with potentially conflicting ports and the `port_base` argument you can provide for `make test`, explained previously.

#### Thread Sanitizer and Testing

Modern C programmers use sanitizers to check their code for correctness and safety issues. The *thread sanitizer (TSAN)* is used to check multithreaded code for race conditions.

We have incorporated all of the necessary setup into the Part 1 `Makefile` for you to check your code with TSAN. Along with the ordinary `http_server` executable, a second TSAN-enabled executable is also produced by the `make` command: `http_server_tsan`. You can run this program just like the ordinary server program. However, if a race condition is detected, the TSAN version will print out an informative error message for you to review.

We have not provided a test case for `http_server_tsan`, but we may run your code through TSAN as a hidden autograder test, so it's advisable that you use this tool and fix any errors that TSAN reports before submitting.

If you are experiencing issues with TSAN on a Windows machine, recall from Lab 7 that there is a fix involving the `setarch` command. For example: to apply the fix, designate the `server_files` directory as the location of files to provide to clients, and specify port `4061`, you can run the command:

``` sh
setarch -R ./http_server_tsan server_files 4061
```

## Part 2

Now, you will implement a multi-threaded HTTP server in Python rather than in C. Hopefully, you will find this part of the project significantly shorter than Part 1, both because of the abstractions offered by Python and because you will have become familiar with HTTP and multithreading from your work in Part 1.

### A Note on Python Libraries

Unlike C, Python has a rich set of libraries to tackle many systems programming tasks. Some Python libraries are highly abstract and can make writing an HTTP server trivial, while others are simple wrappers for system calls and can make your code nearly as complex as it would be in C.

The approach we describe below, and that you are **required** to follow, should strike a reasonable middle ground. You'll be able to avoid much of the tedium of C but still get a sense of how to build things for yourself. This can be valuable because, in the future, you might want to build something in Python with requirements that don't align neatly with the standard library's abstractions (and therefore constraints), in which case you'll need to work directly with the relevant system primitives.

It's worth your time to study Python's `socket` library carefully. It has many helpful functions you are free to use. Make sure you are reading documentation specifically for Python version 3.10, which is the version used in the 4061 container.

### HTTP Utility Functions

The `http_handler.py` file contains functions for reading HTTP requests and writing HTTP responses. Just like in C, `read_request` should read in a request and return the resource being requested. You shouldn't need any especially fancy functions for the string handling to extract the resource name from the HTTP request.

The `write_response` function is also similar to its C counterpart in Part 1. It accepts a file path and should either write a response containing the file's contents if it exists, or sends back a `Not Found` response (error code `404`) otherwise.

There are some helpful Python library functions you may want to investigate for:

- Reading input from a socket one line at a time (see the `socket` library)
- Conveniently sending file contents over a socket (see the `socket` library)
- Checking if a file exists and obtaining the file's size (see the `pathlib` library)

*Hint*: Python distinguishes between strings and byte arrays. You can only send a byte array, not a string, over a socket. To convert a string literal to a byte array, you can prefix it with `b`, e.g., `b"Hello, World!"`. Or if you have a string variable `s`, you can use `s.encode("utf-8")` to convert it to a byte array.

### Connection Queue

We'll use Python's OOP features and define a class to represent a connection queue. It will have `enqueue`, `dequeue`, and `shut_down` methods that work the same way as your C code back in Part 1. You can safely take a very similar approach to synchronization here as you did in Part 1, meaning you can use the same combination of locks and condition variables. Just be attentive to the differences in the APIs for these primitives between C and Python.

The enqueue and dequeue operations will block or proceed under the same set of rules as we saw in Part 1. Similarly, shutting down a queue should unblock any threads that were waiting in a call to `enqueue` or `dequeue`. However, instead of returning an error was we did in C, the Python code should raise a `ShutDownException`. We have already defined this exception type for you at the top of `connection_queue.py`.

One other simplification we make compared to Part 1 and C: We'll just store queue elements in a `deque` (double-ended queue) data structure from Python's `collections` library. Read up on its features. You can efficiently add items to the end of the deque (for enqueue operations) and remove items from the beginning of the deque (for dequeue operations).

You are **not allowed** to use any Python libraries beyond those already imported for you in the starter code (`collections`, `socket` and `threading`).

### HTTP Server

You are **not allowed** to use any Python libraries beyond those already imported for you in the starter code (`signal`, `socket`, `sys`, and `threading`).

Your Python server should follow the same structure as the C server:

- Create a fixed-size pool of worker threads using the `threading` library
- Create a queue to pass connections from the main thread to worker threads
- In the main loop, accept a new client connection, store it in the shared queue, and repeat
- Within a worker thread, loop to dequeue a client connection socket, read the client's request, then write an appropriate response back to the connection
- If a worker thread encounters a `ShutDownException`, it should break out of its loop and stop.
- When `SIGINT` is delivered to the server, stop the main loop, shut down the shared queue, and join all worker threads.

Python's `socket` library has functions that wrap all the same system calls we saw in C: `getaddrinfo`, `bind`, `listen`, `accept`, etc. You are welcome to use these. However, the `socket` library also offers a more convenient way to create a server socket, specifically. You may want to find and use this alternative!

#### Python Signal Handling

Python signal handling is both easier and harder than in C. The good news:

- You can just use the `signal` function from the `signal` library, no need to bother with the complexity of `sigaction`
- Python will deliver signals to the main thread, so you don't need to worry about blocking signals before creating the worker threads and unblocking them afterwards, as you did in C.
- We've already provided you with a suitable signal handler function in the starter code.

The problem is that Python doesn't deliver signals immediately to your code when it is in the middle of a blocking system call like `accept`. Instead, it only delivers them once the system call returns. So if you were to structure your Python code exactly like your C code in Part 1, your server would stay stuck in an `accept` call even after receiving `SIGINT`.

How can you fix this?

1.  Use the `settimeout` method on your server socket before calling its `accept` method. Set the timeout for something like 1 second.
2.  Put your call to `accept` in a `try` block, and then catch (`except`) specifically a `TimeoutError`. If this error occurs, just repeat your loop and call `accept` again.
3.  If you get out of the `try` block, it means `accept` succeeded, and you need to enqueue the new connection.

This essentially allows Python to regain control from the `accept` system call after the timeout expires and, if `SIGINT` has arrived, it will react accordingly by calling your signal handler. Otherwise, we just get a `TimeoutError` and repeat the `accept`.

### Testing the Server

We have provided the same test cases for the Python HTTP server as were provided for the C server. The first tests out sequential requests (so you can get this working without threads or the connection queue), and the second tests out requests in parallel, again using the `LD_PRELOAD` trick to verify that your program is actually executing code from multiple threads.

## Submission

You must submit your code to Gradescope to receive credit for this project. Run the `make zip` command to create a zip archive suitable for uploading to Gradescope.

**Remember, you can only modify the files designated as *EDIT* in the table above.** Modifying other files will cause your code to fail the autograder tests.

**It is your responsibility to ensure that the Gradescope autograder results match your expectations based on local testing.**

You may also wish to review the late submission policy detailed in the [course syllabus](https://canvas.umn.edu/courses/579183/assignments/syllabus) to understand what score you will receive if submitting your work after the deadline. **No project submissions are accepted more than 48 hours after the deadline.**

### Intermittent Gradescope Test Failures

You may observe that submitting the same code to the autograder multiple times produces different results. There are two main reasons for this:

1.  Your code is not thread safe, and there is a bug that only makes itself known when threads are scheduled in a certain order. Use mutexes and condition variables to make your code thread-safe, meaning it will *always* behave correctly.
2.  Your code attempted to bind to a port that was already claimed by a different process on the autograder server. Unfortunately, Gradescope has set up its infrastructure such that your code is running on a shared machine with other code, which may try to bind to the same port. This was a decision made by the Gradescope backend engineers and is out of our control.

In light of these potential issues, you can regard your autograder-assigned score as provisional. After the project deadline, course staff will download all student submissions and run each submission **once**, making sure to use a free port for your server. This means that:

1.  If your code has thread safety issues and you spam the autograder until you get the result you want, you'll likely fail our final round of testing. Fix your code so that it has consistent behavior across different runs.
2.  If you run into port reservation issues on the autograder, you don't need to worry about these affecting your test results. We'll ensure this doesn't occur in our final test run.
