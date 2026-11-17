# CSCI 4061 -- Quiz 09 (answered)

> Archived from the Canvas quiz page
> <https://canvas.umn.edu/courses/579183/quizzes/1255583>
>
> | | |
> |---|---|
> | **Due** | Mon Nov 16, 2026 11:59pm |
> | **Points** | 10 (10 questions, 1 pt each) |
> | **Available** | Nov 9, 2026 12:00am - Nov 16, 2026 11:59pm |
> | **Allowed attempts** | 10 |
> | **Time limit** | none |

> **Note:** Answered copy of `QUESTIONS.md`; every choice cites the Week 9 slides, Lec16 *More Networking* (Nov 3) and Lec17 *TCP, HTTP* (Nov 5), or the Lab 9 slides. Q1 and Q2 also cite the networking half of Lec15 (Oct 29), where TCP, UDP and ports were introduced. Q4-Q6, Q8 and Q10 were also checked with a small C program in the devcontainer.

---

## Quiz Instructions

This quiz focuses on course content introduced in the lectures of Week 9, the reading assignments associated with these lectures, and Lab 9. To prepare for this quiz, make sure you are familiar with these materials, and review them as necessary.

Credit is earned for this quiz when you submit your answers. You may submit up to ten times before the quiz deadline (usually one week after the quiz's release date).

## Questions

### Question 1

Which of the following is something that TCP and UDP have in common?

- [ ] TCP and UDP both offer reliable delivery of data.
- [ ] TCP and UDP both allow data to be sent in arbitrarily long byte sequences.
- [ ] TCP and UDP both require a connection to be set up before communication can occur.
- [X] TCP and UDP both rely on IP to send data to the destination address.

> **Why:** Lec15 slide 31: UDP is "Built on top of IP (intentionally adds very little on top)"; slide 32: TCP is "Built on top of IP, but much more application-friendly". The other three are TCP-only. Slide 30: UDP sends "fixed-size chunks (datagrams) unreliably", while TCP gives "Reliable (no loss), in-order transmission of arbitrarily long sequence of bytes". Slide 31 calls UDP "Conectionless: Send it and forget it", and slide 32 calls TCP "Connection-Oriented". The Lab 9 slides' "TCP vs. UDP" table says the same.

### Question 2

What is the purpose of a port in TCP and in UDP?

- [ ] It lets us set different priority levels for different communications.
- [ ] It allows us to use special features of TCP and UDP not available in IP.
- [X] It disambiguates which process on a computer should receive data.
- [ ] It serves as a refuge when network traffic storms occur.

> **Why:** Lec15 slide 36: "When I send data to an IP address, how do I disambiguate which process should receive it? TCP and UDP have ports - unique numbers assigned to the different processes running on a "host"". Lec16 slide 6 repeats it under the title "Ports - Picking a Process": the IP address picks the computer, the port picks the process.

### Question 3

Why doesn't a server need to know a client's IP address?

- [ ] The server can use DNS to look up the client's address
- [X] A server will learn a client's address when it receives a request from the client.
- [ ] The server only needs to know the client's TCP or UDP address to send a reply.
- [ ] Trick question: Since servers have to initiate contact with clients, they actually do need to know a client's IP address.

> **Why:** Lec16 slide 10: the client "Needs to know server's address", while the server "Doesn't initiate contact with clients", which rules out choice 4. For UDP, `recvfrom()`'s last two arguments "Tells you where UDP message is from - Useful if you want to send back a reply" (Lec16 slide 19). The lecture's `question_server_udp.c` does exactly that: it passes the `client_addr` filled in by `recvfrom()` to `sendto()`. For TCP, `accept()`'s `addr` "Gives address of connecting client" (Lec17 slide 7). Choice 3 is wrong because a port alone doesn't identify the client: a connection is the source and destination IP address, port number and protocol (Lec16 slide 7). Choice 1 is wrong because DNS maps a host *name* to an address (Lec16 slide 4), and the server doesn't know the client's name in advance either.

### Question 4

Which of the following is done through the `getaddrinfo()` system call?

- [ ] DNS lookup to convert from the destination host's name to its IP address
- [ ] Service lookup to convert a service name (e.g., `"http"`) to the appropriate port number
- [ ] Population of a `struct addrinfo` with values that are used as parameters for several other networking system calls
- [X] All of the above

> **Why:** Lec16 slide 13: given a human-readable name for `node`, getaddrinfo "does the necessary DNS lookup for us", and given a service name like "http" for `service`, "the name is mapped to the standard port number for us". Slide 12: it "Does everything we need to pass correct parameters to socket() and later system calls", and later slides pass the filled-in fields (slide 15) to `socket()` (slide 17), `sendto()` (slide 18), `bind()` (slide 22) and `connect()` (Lec17 slide 3). Tested: `getaddrinfo("localhost", "http", ...)` returned `::1` and `127.0.0.1`, both with port 80.

### Question 5

What data structure is used by `getaddrinfo()` to return multiple results?

- [ ] A Hash Table
- [ ] An Array
- [X] A Linked List
- [ ] A Vector

> **Why:** Lec16 slide 15: `struct addrinfo` ends with `struct addrinfo *ai_next;` because "getaddrinfo() may want to return more than one struct addrinfo". Each result points to the next. Tested: walking `ai_next` from the result of the Q4 call visited 2 results, with `ai_next` NULL on the last.

### Question 6

Which pair of system calls are used to exchange traffic with another process using UDP?

- [X] `sendto()` and `recvfrom()`
- [ ] `send()` and `recv()`
- [ ] `read()` and `write()`
- [ ] `malloc()` and `free()`

> **Why:** Lec16 slides 18-19: "UDP sockets don't use read() and write()", followed by the signatures of `sendto()` and `recvfrom()`. Slide 20's demo client and server use `sendto()`/`recvfrom()`, and the Lab 9 table says "every sendto() names the receiver". Tested: `sendto()`/`recvfrom()` delivered a datagram between two UDP sockets, while `send()` on the same unconnected socket failed with "Destination address required". This doesn't change the answer: after `connect()` on a UDP socket, `send()`/`recv()` also worked. The course never covers that.

### Question 7

Which port is reserved for use by DNS?

- [X] 53
- [ ] 70
- [ ] 80
- [ ] 443

> **Why:** Lec16 slide 8: "SSH Server: 22, DNS Server: 53, HTTP Server: 80, HTTPS: 443". Slide 9's steps for visiting a website start with a UDP message to a DNS server on "port 53". The container's `/etc/services` lists `domain 53/tcp` and `53/udp`.

### Question 8

When a TCP client attempts to connect to a non-existent server process, which of the following system calls will return an error?

- [ ] `socket()`
- [X] `connect()`
- [ ] `bind()`
- [ ] `accept()`

> **Why:** Lec17 slide 3: "Connection is "synchronous" - must be someone else on the other side. Will return an error if server doesn't accept your connection request". `socket()` only creates a file descriptor and doesn't contact anyone (Lec16 slide 11), and `bind()` and `accept()` are server-side calls (Lec17 slide 8). Tested: `socket()` returned fd 3, then `connect()` to `127.0.0.1:4999`, where nothing was listening, returned -1 with "Connection refused".

### Question 9

What is the purpose of the second argument of the `listen()` system call?

- [ ] It specifies which port the server socket will be listening on for incoming client requests.
- [ ] It specifies if the server socket will use UDP or TCP.
- [ ] It specifies if the server socket will use IPv4 or IPv6.
- [X] It specifies the maximum number of clients that the server can keep waiting to start their connections.

> **Why:** Lec17 slide 6: `int listen(int sockfd, int backlog)`, where "backlog: Maximum number of queued clients". listen(2): "The backlog argument defines the maximum length to which the queue of pending connections for sockfd may grow." The other three are set elsewhere: the port by `bind()` (Lec16 slides 21-22), and TCP vs UDP and IPv4 vs IPv6 by `socket()`'s type and domain (Lec16 slide 11).

### Question 10

Say we are running a TCP server that ends up connecting with `n` clients. Which of the following system calls is invoked `n` times when this occurs?

- [ ] `getaddrinfo()`
- [ ] `socket()`
- [ ] `bind()`
- [ ] `listen()`
- [X] `accept()`

> **Why:** Lec17 slide 7: `accept()` "Waits for a new client to request a TCP connection" and "returns a new file descriptor specific to this client". Slide 6: the server keeps "One descriptor to listen for new clients" plus "One new file descriptor per client". In slides 8-9, `getaddrinfo()`, `socket()`, `bind()` and `listen()` run once during setup. The lecture's `question_server_tcp.c` calls `accept()` inside its `while` loop and everything else before it. Tested: a server with 3 clients made 1 call each to `getaddrinfo()`, `socket()`, `bind()` and `listen()`, and 3 calls to `accept()`.
