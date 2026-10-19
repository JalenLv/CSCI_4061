// SPDX-FileCopyrightText: 2026 John Kolb <jhkolb@umn.edu>
// SPDX-License-Identifier: GPL-3.0-or-later
#include <Python.h>
#include <stdio.h>

// Function definition at the end of this file
PyObject *square_roots(PyObject *self, PyObject *args);

static PyMethodDef square_root_methods[] = {
    {
        .ml_name = "square_roots",
        .ml_meth = square_roots,
        .ml_flags = METH_VARARGS,
        .ml_doc = "Compute square roots of all numbers in a list",
    },
    {
        .ml_name = NULL,
        .ml_meth = NULL,
        .ml_flags = 0,
        .ml_doc = NULL,
    }};

static struct PyModuleDef square_root_module = {
    .m_base = PyModuleDef_HEAD_INIT,
    .m_name = "square_root_numbers",
    .m_doc = "Compute square roots of all numbers in a list",
    .m_size = -1,
    .m_methods = square_root_methods,
};

PyMODINIT_FUNC PyInit_square_root(void) {
    return PyModule_Create(&square_root_module);
}

PyObject *square_roots(PyObject *self, PyObject *args) {
    // TODO Complete this function
    return NULL;
}
