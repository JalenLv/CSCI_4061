#!/usr/bin/env python3
"""Generates the small matrices and expected outputs used by tests_extra.json.

Run from anywhere; it writes next to itself (resources/*.bin) and into ../output/.
Re-running it reproduces the committed files byte for byte.

Matrix file format (the one read_matrix_from_file() expects, native little-endian):
    uint32 rows, uint32 cols, then rows * cols int32 elements in row-major order.

Expected output format (the one print_matrix() produces):
    one line per row, elements printed with %d and separated by ", ".
"""
import os
import struct

HERE = os.path.dirname(os.path.abspath(__file__))
OUTPUT = os.path.join(HERE, "..", "output")

# Small pairs, so that more workers than elements is easy to reach. Values stay
# below 100 in magnitude, so no product comes anywhere near the int32 range.
PAIRS = {
    "tiny_1x1": ([[-7]], [[6]]),
    "tiny_1x5": ([[1, -2, 3, -4, 5]], [[10, 20, -30, 40, -50]]),
    "tiny_5x1": ([[3], [-3], [0], [99], [-99]], [[3], [3], [99], [-99], [-99]]),
    "tiny_3x4": (
        [[1, 2, 3, 4], [5, 6, 7, 8], [9, 10, 11, 12]],
        [[-1, 2, -3, 4], [0, 1, 0, -1], [12, -11, 10, -9]],
    ),
    "tiny_2x3": ([[-1, -2, -3], [4, 5, 6]], [[7, -8, 9], [-10, 11, -12]]),
}

# Every mode with 7 and with 50 workers, in the order the tiny-matrix tests call them
ALL_MODES_CALLS = ["pipe:7", "pipe:50", "shmem:7", "shmem:50", "threads:7", "threads:50"]


def write_matrix(path, rows):
    data = struct.pack("<II", len(rows), len(rows[0]))
    for row in rows:
        data += struct.pack(f"<{len(row)}i", *row)
    with open(path, "wb") as f:
        f.write(data)


def product_text(a, b):
    lines = []
    for row_a, row_b in zip(a, b):
        lines.append(", ".join(str(x * y) for x, y in zip(row_a, row_b)))
    return "\n".join(lines) + "\n"


def write_output(name, text):
    with open(os.path.join(OUTPUT, name), "w") as f:
        f.write(text)


def main():
    os.makedirs(OUTPUT, exist_ok=True)
    products = {}
    for name, (a, b) in PAIRS.items():
        write_matrix(os.path.join(HERE, f"{name}_a.bin"), a)
        write_matrix(os.path.join(HERE, f"{name}_b.bin"), b)
        products[name] = product_text(a, b)
        # Tests 16-20: the same product once per call in ALL_MODES_CALLS
        write_output(f"{name}_all_modes.txt", products[name] * len(ALL_MODES_CALLS))

    # Same element count as tiny_2x3 (6) but transposed shape: only a real
    # dimension check (not an element-count check) rejects this pair
    write_matrix(os.path.join(HERE, "tiny_3x2_b.bin"), [[1, 2], [3, 4], [5, 6]])

    # Header promises 3x4 = 12 elements, but only 5 follow
    with open(os.path.join(HERE, "truncated_3x4.bin"), "wb") as f:
        f.write(struct.pack("<II", 3, 4) + struct.pack("<5i", 1, 2, 3, 4, 5))

    # Test 24: threads, pipe, shmem, threads on tiny_3x4 with stdout piped to cat
    write_output("stdout_pipe_no_duplicates.txt", products["tiny_3x4"] * 4)

    zero = "ValueError: Number of workers must be at least 1\n"
    too_big = "OverflowError: signed integer is greater than maximum\n"
    # Test 25: 0 workers in each mode, -1 in threads mode, 2**32 (too big for the "i"
    # format) in each mode, then one successful call
    write_output("bad_worker_counts.txt", zero * 4 + too_big * 3 + products["tiny_1x1"])

    missing = "FileNotFoundError: [Errno 2] No such file or directory\n"
    # Test 26: missing A, then missing B, for each mode, then one successful call
    write_output("missing_file.txt", missing * 6 + products["tiny_1x1"])

    # Test 27: truncated B for each mode, then one successful call
    truncated = "OSError: Unable to read from matrix data file\n"
    write_output("truncated_file.txt", truncated * 3 + products["tiny_1x1"])

    # Test 28: 2x3 times 3x2 for each mode, then one successful call
    mismatch = "ValueError: Matrix dimensions do not match\n"
    write_output("mismatched_dims.txt", mismatch * 3 + products["tiny_1x1"])

    # Test 29: successes and the failures the spec defines (missing and truncated
    # files) in every mode, then the leak report
    write_output(
        "no_leaks.txt",
        (products["tiny_2x3"] + missing + missing + truncated) * 3
        + products["tiny_2x3"]
        + "leaked file descriptors: 0\n"
        + "leaked shared memory mappings: 0\n"
        + "child processes left: none\n",
    )

    # Tests 21-23 compare mat_03 byte for byte with cmp, which prints nothing on a match
    write_output("identical.txt", "identical\n")

    # Test 30: valgrind_leaks.sh, pipe, shmem and threads on tiny_2x3, then its summary
    write_output(
        "valgrind_leaks.txt",
        products["tiny_2x3"] * 3
        + "processes checked by valgrind: 7\n"
        + "processes with memory errors: 0\n"
        + "leak records allocated from hadamard_{pipe,shmem,threads}.c: 0\n",
    )


if __name__ == "__main__":
    main()
