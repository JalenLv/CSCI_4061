#!/usr/bin/env python3
"""Raw-socket HTTP client for the extra tests. Every subcommand prints a short,
deterministic summary of what it saw, so testius can diff it.

  http_client.py check PORT RESOURCE [FILE] [--chunk N --delay S] [--end-at OFFSET]
      Send "GET RESOURCE HTTP/1.0", read until the server closes, print the status
      line, Content-Type, Content-Length, and whether the body matches FILE.
      --chunk/--delay: send the request N bytes at a time, S seconds apart.
      --end-at: pad the headers so the closing "\\r\\n\\r\\n" starts at OFFSET.
  http_client.py slow PORT RESOURCE FILE DELAY
      Connect, wait DELAY seconds, then do the same as check.
  http_client.py abort PORT RESOURCE KEEP
      Request RESOURCE, read KEEP bytes, close with the rest unread.
  http_client.py rude PORT
      Three misbehaving clients: connect and close; half a request then close;
      a malformed request line (prints whatever comes back).
  http_client.py many PORT DIR N
      N sequential requests cycling through every file in DIR plus a missing one.
  http_client.py wait-listen PORT
      Wait (up to 5 s) until something listens on PORT, without connecting to it.
  http_client.py threads PID / fds PID
      Print the number of threads / open file descriptors of process PID.
  http_client.py masks PID
      Say whether the main thread and the other threads of PID block SIGINT.
"""
import os
import socket
import sys
import time

TIMEOUT = 10


def build_request(resource, end_at=None):
    head = f"GET {resource} HTTP/1.0\r\nHost: localhost\r\nUser-Agent: extra-tests\r\n"
    if end_at is not None:
        # One filler header sized so the final "\r\n\r\n" starts at end_at
        filler = end_at - len(head) - len("X-Filler: ")
        if filler < 0:
            sys.exit(f"--end-at {end_at} is too small")
        head += "X-Filler: " + "a" * filler + "\r\n"
    request = (head + "\r\n").encode()
    assert end_at is None or request.find(b"\r\n\r\n") == end_at
    return request


def read_all(sock):
    data = b""
    try:
        while True:
            chunk = sock.recv(65536)
            if not chunk:
                return data, "closed"
            data += chunk
    except socket.timeout:
        return data, f"no EOF within {TIMEOUT} s"
    except ConnectionResetError:
        return data, "connection reset"


def describe(resource, data, ending, file):
    out = [f"GET {resource}:"]
    head, sep, body = data.partition(b"\r\n\r\n")
    if not sep:
        out.append(f"no complete header in {len(data)} bytes ({ending})")
        return " ".join(out)
    lines = head.split(b"\r\n")
    if any(b"\n" in line for line in lines):
        out.append("[bare \\n inside the header]")
    out.append(lines[0].decode(errors="replace"))
    headers = {}
    for line in lines[1:]:
        name, _, value = line.decode(errors="replace").partition(":")
        headers[name.strip().lower()] = value.strip()
    for name in ("Content-Type", "Content-Length"):
        if name.lower() in headers:
            out.append(f"| {name}: {headers[name.lower()]}")
    out.append(f"| body {len(body)} bytes")
    if file is not None:
        with open(file, "rb") as f:
            out.append("matching" if f.read() == body else "NOT matching")
        out.append(os.path.basename(file))
    out.append(f"| {ending}")
    return " ".join(out)


def connect(port):
    try:
        return socket.create_connection(("localhost", int(port)), timeout=TIMEOUT)
    except OSError as e:
        print(f"could not connect to port {port}: {e}")
        sys.exit(1)


def check(port, resource, file=None, chunk=None, delay=0.0, end_at=None, pre_delay=0.0):
    request = build_request(resource, end_at)
    with connect(port) as sock:
        if pre_delay:
            time.sleep(pre_delay)
        try:
            if chunk is None:
                sock.sendall(request)
            else:
                sock.setsockopt(socket.IPPROTO_TCP, socket.TCP_NODELAY, 1)
                for i in range(0, len(request), chunk):
                    sock.sendall(request[i:i + chunk])
                    time.sleep(delay)
        except OSError as e:
            print(f"GET {resource}: send failed: {e}")
            return
        data, ending = read_all(sock)
    print(describe(resource, data, ending, file))


def abort(port, resource, keep):
    with connect(port) as sock:
        sock.sendall(build_request(resource))
        got = 0
        while got < keep:
            chunk = sock.recv(min(65536, keep - got))
            if not chunk:
                break
            got += len(chunk)
    print(f"GET {resource}: read {got} bytes, then closed")


def rude(port):
    with connect(port):
        pass
    print("client 1: connected and closed without sending anything")
    with connect(port) as sock:
        sock.sendall(b"GET /quote.txt HTTP/1.0\r\nHost: loc")
    print("client 2: sent half a request and closed")
    with connect(port) as sock:
        sock.sendall(b"\xff\xfe nonsense\r\n\r\n")
        _, ending = read_all(sock)
    # Whether to answer at all is up to the server; it must not leave the client hanging
    print(f"client 3: sent a malformed request line, then the connection was "
          f"{'closed' if ending in ('closed', 'connection reset') else ending}")


def many(port, directory, n):
    files = sorted(os.listdir(directory))
    contents = {}
    for name in files:
        with open(os.path.join(directory, name), "rb") as f:
            contents[name] = f.read()
    names = files + ["affordable_gpu.txt"]
    ok = not_found = 0
    for i in range(n):
        name = names[i % len(names)]
        try:
            with connect(port) as sock:
                sock.sendall(build_request("/" + name))
                data, ending = read_all(sock)
        except OSError as e:
            print(f"request {i + 1} ({name}) failed: {e}")
            break
        head, _, body = data.partition(b"\r\n\r\n")
        status = head.split(b"\r\n")[0]
        if name in contents and status == b"HTTP/1.0 200 OK" and body == contents[name]:
            ok += 1
        elif name not in contents and status == b"HTTP/1.0 404 Not Found" and body == b"":
            not_found += 1
        else:
            print(f"request {i + 1} ({name}): wrong response {status!r}, "
                  f"{len(body)} body bytes ({ending})")
            break
    print(f"{n} requests: {ok} correct 200s, {not_found} correct 404s")


def listening(port):
    hexport = f"{int(port):04X}"
    for table in ("/proc/net/tcp", "/proc/net/tcp6"):
        try:
            with open(table) as f:
                next(f)
                for line in f:
                    fields = line.split()
                    if fields[1].endswith(":" + hexport) and fields[3] == "0A":
                        return True
        except FileNotFoundError:
            pass
    return False


def wait_listen(port):
    deadline = time.monotonic() + 5
    while time.monotonic() < deadline:
        if listening(port):
            return
        time.sleep(0.05)
    print(f"nothing listening on port {port} after 5 s")
    sys.exit(1)


def masks(pid):
    blocked = {}
    for tid in os.listdir(f"/proc/{pid}/task"):
        with open(f"/proc/{pid}/task/{tid}/status") as f:
            sigblk = next(line for line in f if line.startswith("SigBlk:")).split()[1]
        blocked[tid] = bool(int(sigblk, 16) & (1 << 1))  # Bit n-1 is signal n; SIGINT is 2
    others = [b for tid, b in blocked.items() if tid != str(pid)]
    print(f"main thread blocks SIGINT: {'yes' if blocked[str(pid)] else 'no'}; "
          f"other threads blocking SIGINT: {sum(others)} of {len(others)}")


def main():
    args = sys.argv[1:]
    opts = {}
    for flag in ("--chunk", "--delay", "--end-at"):
        if flag in args:
            i = args.index(flag)
            opts[flag] = args[i + 1]
            del args[i:i + 2]
    cmd = args[0]
    if cmd == "check":
        check(args[1], args[2], args[3] if len(args) > 3 else None,
              chunk=int(opts["--chunk"]) if "--chunk" in opts else None,
              delay=float(opts.get("--delay", 0)),
              end_at=int(opts["--end-at"]) if "--end-at" in opts else None)
    elif cmd == "slow":
        check(args[1], args[2], args[3], pre_delay=float(args[4]))
    elif cmd == "abort":
        abort(args[1], args[2], int(args[3]))
    elif cmd == "rude":
        rude(args[1])
    elif cmd == "many":
        many(args[1], args[2], int(args[3]))
    elif cmd == "wait-listen":
        wait_listen(args[1])
    elif cmd == "threads":
        print(len(os.listdir(f"/proc/{args[1]}/task")))
    elif cmd == "fds":
        print(len(os.listdir(f"/proc/{args[1]}/fd")))
    elif cmd == "masks":
        masks(args[1])
    else:
        sys.exit(__doc__)


if __name__ == "__main__":
    main()
