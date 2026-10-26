#!/usr/bin/env python3
"""Calls the hadamard extension directly, without hadamard_main.py's checks.

usage: hadamard_call.py [--check-leaks] <mat_a> <mat_b> <call> [<call> ...]

Each <call> is <mode>:<n_workers> or <mode>:<n_workers>:<mat_a>:<mat_b>, and runs
hadamard.hadamard_<mode>(mat_a, mat_b, n_workers) in this one Python process, in the
order given. The second form overrides the two files for that call only. n_workers is
any integer, including 0, negative numbers and values too big for a C int.

An exception from a call is printed as "<ExceptionType>: <message>" and the next call
still runs. With --check-leaks, it finally prints how many more file descriptors and
shared memory mappings exist than before the first call, and whether any child process
is left (running or unreaped).
"""
import os
import sys

# hadamard.cpython-*.so is built in part2/, three directories up from this script
sys.path.insert(0, os.path.join(os.path.dirname(os.path.abspath(__file__)), "..", "..", ".."))
import hadamard  # noqa: E402


def open_fd_count():
    return len(os.listdir("/proc/self/fd"))


def shared_mapping_count():
    # Linux lists MAP_SHARED | MAP_ANONYMOUS mappings as "/dev/zero (deleted)"
    with open("/proc/self/maps") as f:
        return sum(1 for line in f if "/dev/zero" in line)


def children_left():
    try:
        os.waitpid(-1, os.WNOHANG)
    except ChildProcessError:
        return False  # ECHILD: no children at all, running or zombie
    return True


def main():
    args = sys.argv[1:]
    check_leaks = bool(args) and args[0] == "--check-leaks"
    if check_leaks:
        args = args[1:]
    if len(args) < 3:
        print(__doc__)
        sys.exit(1)

    mat_a, mat_b, calls = args[0], args[1], args[2:]
    fds_before = open_fd_count()
    mappings_before = shared_mapping_count()
    for call in calls:
        fields = call.split(":")
        mode, n_workers = fields[0], int(fields[1])
        a, b = (fields[2], fields[3]) if len(fields) == 4 else (mat_a, mat_b)
        try:
            getattr(hadamard, f"hadamard_{mode}")(a, b, n_workers)
        except Exception as e:
            print(f"{type(e).__name__}: {str(e).strip()}")

    if check_leaks:
        print(f"leaked file descriptors: {open_fd_count() - fds_before}")
        print(f"leaked shared memory mappings: {shared_mapping_count() - mappings_before}")
        print(f"child processes left: {'yes' if children_left() else 'none'}")


if __name__ == "__main__":
    main()
