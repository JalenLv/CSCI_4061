# SPDX-FileCopyrightText: 2026 John Kolb <jhkolb@umn.edu>
# SPDX-License-Identifier: GPL-3.0-or-later
import square_root

if __name__ == "__main__":
    print("List 1:")
    numbers = [4, 8, 15, 16, 23, 42, 108]
    try:
        results = square_root.square_roots(numbers)
        for r in results:
            print(f"{r:.3f}")
    except ValueError:
        print("Could not compute square roots due to invalid list element")
    print()

    print("List 2:")
    numbers = [4, 8, -15, 16, 23, 42, 108]
    try:
        results = square_root.square_roots(numbers)
        for r in results:
            print(f"{r:.3f}")
    except ValueError as msg:
        print(msg)
