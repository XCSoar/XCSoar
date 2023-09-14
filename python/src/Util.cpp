// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The XCSoar Project

#include <Python.h>

#include "Util.hpp"

#include "PythonConverters.hpp"
#include "Tools/GoogleEncode.hpp"

#if PY_MAJOR_VERSION >= 3
    #define PyInt_AsLong PyLong_AsLong
#endif

PyObject *
xcsoar_encode([[maybe_unused]] PyObject *self,
              PyObject *args, PyObject *kwargs)
{
  PyObject *py_list,
           *py_method = nullptr;
  double floor_to = 1;
  bool delta = true;

  static char *kwlist[] = {"list", "delta", "floor", "method", nullptr};

  if (!PyArg_ParseTupleAndKeywords(args, kwargs, "O|idO", kwlist,
                                   &py_list, &delta, &floor_to, &py_method)) {
    return nullptr;
  }

  if (!PySequence_Check(py_list)) {
    PyErr_SetString(PyExc_TypeError, "Expected a list.");
    return nullptr;
  }

  Py_ssize_t num_items = PySequence_Fast_GET_SIZE(py_list);

  // return empty string if list has no elements
  if (num_items == 0)
    return PyUnicode_FromString("");

  unsigned dimension;
  if (PySequence_Check(PySequence_Fast_GET_ITEM(py_list, 0))) {
    dimension = PySequence_Size(PySequence_Fast_GET_ITEM(py_list, 0));
  } else {
    dimension = 1;
  }

  enum Method { UNSIGNED, SIGNED, DOUBLE } method;

  if (py_method == nullptr)
    method = UNSIGNED;
#if PY_MAJOR_VERSION >= 3
  else if (PyUnicode_Check(py_method) && strcmp(PyUnicode_AsUTF8(py_method), "unsigned") == 0)
    method = UNSIGNED;
  else if (PyUnicode_Check(py_method) && strcmp(PyUnicode_AsUTF8(py_method), "signed") == 0)
    method = SIGNED;
  else if (PyUnicode_Check(py_method) && strcmp(PyUnicode_AsUTF8(py_method), "double") == 0)
    method = DOUBLE;
#else
  else if (PyString_Check(py_method) && strcmp(PyString_AsString(py_method), "unsigned") == 0)
    method = UNSIGNED;
  else if (PyString_Check(py_method) && strcmp(PyString_AsString(py_method), "signed") == 0)
    method = SIGNED;
  else if (PyString_Check(py_method) && strcmp(PyString_AsString(py_method), "double") == 0)
    method = DOUBLE;
#endif
  else {
    PyErr_SetString(PyExc_TypeError, "Can't parse method.");
    return nullptr;
  }

  GoogleEncode encoded(dimension, delta, floor_to);

  for (Py_ssize_t i = 0; i < num_items; ++i) {
    PyObject *py_item = PySequence_Fast_GET_ITEM(py_list, i);

    if (dimension > 1) {
      for (unsigned j = 0; j < dimension; ++j) {

        if (method == UNSIGNED) {
          if (!PyNumber_Check(PySequence_Fast_GET_ITEM(py_item, j))) {
            PyErr_SetString(PyExc_TypeError, "Expected numeric value.");
            return nullptr;
          }
          encoded.addUnsignedNumber(PyInt_AsLong(PySequence_Fast_GET_ITEM(py_item, j)));
        } else if (method == SIGNED) {
          if (!PyNumber_Check(PySequence_Fast_GET_ITEM(py_item, j))) {
            PyErr_SetString(PyExc_TypeError, "Expected numeric value.");
            return nullptr;
          }
          encoded.addSignedNumber(PyInt_AsLong(PySequence_Fast_GET_ITEM(py_item, j)));
        } else if (method == DOUBLE) {
          if (!PyNumber_Check(PySequence_Fast_GET_ITEM(py_item, j))) {
            PyErr_SetString(PyExc_TypeError, "Expected numeric value.");
            return nullptr;
          }
          encoded.addDouble(PyFloat_AsDouble(PySequence_Fast_GET_ITEM(py_item, j)));
        }

      }
    } else {

      if (method == UNSIGNED) {
        if (!PyNumber_Check(py_item)) {
          PyErr_SetString(PyExc_TypeError, "Expected numeric value.");
          return nullptr;
        }
        encoded.addUnsignedNumber(PyInt_AsLong(py_item));
      } else if (method == SIGNED) {
        if (!PyNumber_Check(py_item)) {
          PyErr_SetString(PyExc_TypeError, "Expected numeric value.");
          return nullptr;
        }
        encoded.addSignedNumber(PyInt_AsLong(py_item));
      } else if (method == DOUBLE) {
        if (!PyNumber_Check(py_item)) {
          PyErr_SetString(PyExc_TypeError, "Expected numeric value.");
          return nullptr;
        }
        encoded.addDouble(PyFloat_AsDouble(py_item));
      }

    }
  }

  // prepare output
  PyObject *py_result = PyUnicode_FromString(encoded.asString()->c_str());

  return py_result;
}
