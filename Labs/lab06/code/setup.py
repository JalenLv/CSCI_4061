# SPDX-FileCopyrightText: 2026 John Kolb <jhkolb@umn.edu>
# SPDX-License-Identifier: GPL-3.0-or-later
from setuptools import Extension, setup

module_sources = [
    "square_root.c",
]

# Define the extension module
module = Extension("square_root", sources=module_sources)

setup(
    name="square_root",
    version="1.0",
    description="Compute square roots of numbers in a list",
    ext_modules=[module],
)
