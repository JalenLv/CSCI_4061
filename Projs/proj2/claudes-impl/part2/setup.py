# SPDX-FileCopyrightText: 2026 John Kolb <jhkolb@umn.edu>
# SPDX-License-Identifier: GPL-3.0-or-later
from setuptools import Extension, setup

module_sources = [
    "hadamard_extension.c",
    "hadamard_utils.c",
    "hadamard_pipe.c",
    "hadamard_shmem.c",
    "hadamard_threads.c",
]

# Define the extension module
module = Extension(
    "hadamard", sources=module_sources, extra_compile_args=["-Wall", "-Werror"]
)

setup(
    name="hadamard",
    version="1.0",
    description="Compute hadamard product of two matrices in parallel",
    ext_modules=[module],
)
