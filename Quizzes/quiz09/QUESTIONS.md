# CSCI 4061 -- Quiz 09

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
- [ ] TCP and UDP both rely on IP to send data to the destination address.

### Question 2

What is the purpose of a port in TCP and in UDP?

- [ ] It lets us set different priority levels for different communications.
- [ ] It allows us to use special features of TCP and UDP not available in IP.
- [ ] It disambiguates which process on a computer should receive data.
- [ ] It serves as a refuge when network traffic storms occur.

### Question 3

Why doesn't a server need to know a client's IP address?

- [ ] The server can use DNS to look up the client's address
- [ ] A server will learn a client's address when it receives a request from the client.
- [ ] The server only needs to know the client's TCP or UDP address to send a reply.
- [ ] Trick question: Since servers have to initiate contact with clients, they actually do need to know a client's IP address.

### Question 4

Which of the following is done through the `getaddrinfo()` system call?

- [ ] DNS lookup to convert from the destination host's name to its IP address
- [ ] Service lookup to convert a service name (e.g., `"http"`) to the appropriate port number
- [ ] Population of a `struct addrinfo` with values that are used as parameters for several other networking system calls
- [ ] All of the above

### Question 5

What data structure is used by `getaddrinfo()` to return multiple results?

- [ ] A Hash Table
- [ ] An Array
- [ ] A Linked List
- [ ] A Vector

### Question 6

Which pair of system calls are used to exchange traffic with another process using UDP?

- [ ] `sendto()` and `recvfrom()`
- [ ] `send()` and `recv()`
- [ ] `read()` and `write()`
- [ ] `malloc()` and `free()`

### Question 7

Which port is reserved for use by DNS?

- [ ] 53
- [ ] 70
- [ ] 80
- [ ] 443

### Question 8

When a TCP client attempts to connect to a non-existent server process, which of the following system calls will return an error?

- [ ] `socket()`
- [ ] `connect()`
- [ ] `bind()`
- [ ] `accept()`

### Question 9

What is the purpose of the second argument of the `listen()` system call?

- [ ] It specifies which port the server socket will be listening on for incoming client requests.
- [ ] It specifies if the server socket will use UDP or TCP.
- [ ] It specifies if the server socket will use IPv4 or IPv6.
- [ ] It specifies the maximum number of clients that the server can keep waiting to start their connections.

### Question 10

Say we are running a TCP server that ends up connecting with `n` clients. Which of the following system calls is invoked `n` times when this occurs?

- [ ] `getaddrinfo()`
- [ ] `socket()`
- [ ] `bind()`
- [ ] `listen()`
- [ ] `accept()`
