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
    PyObject *arg_list;
    if (!PyArg_ParseTuple(args, "O!", &PyList_Type, &arg_list))
        return NULL;

    int list_len = PyList_Size(arg_list);
    // `result_list` owned
    PyObject *result_list = PyList_New(list_len);
    if (!result_list)
        return NULL;

    for (int i = 0; i < list_len; i++) {
        // `curr_obj` borrowed
        PyObject *curr_obj = PyList_GetItem(arg_list, i);
        if (!PyLong_Check(curr_obj)) {
            PyErr_SetString(
                PyExc_ValueError,
                "Could not compute square roots due to invalid list element");
            // `result_list` released
            Py_DecRef(result_list);
            return NULL;
        }

        double curr = PyLong_AsDouble(curr_obj);
        if (curr < 0) {
            PyErr_SetString(PyExc_ValueError,
                            "Cannot compute square root of negative value");
            // `result_list` released
            Py_DecRef(result_list);
            return NULL;
        }

        double result = sqrt(curr);
        // `result_obj` owned
        PyObject *result_obj = PyFloat_FromDouble(result);

        // `result_obj` moved
        PyList_SetItem(result_list, i, result_obj);
    }

    // `result_list` moved
    return result_list;
}
