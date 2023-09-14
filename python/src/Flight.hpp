// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The XCSoar Project

#pragma once

#include <Python.h>
#include <structmember.h> /* required for PyMemberDef */

#include "PythonGlue.hpp"
#include "Flight/Flight.hpp"

/* xcsoar.Flight methods */
struct Pyxcsoar_Flight {
  PyObject_HEAD Flight *flight;
  char *filename;
};

PyObject* xcsoar_Flight_new(PyTypeObject *type, PyObject *args, PyObject *kwargs);
void xcsoar_Flight_dealloc(Pyxcsoar_Flight *self);

PyObject* xcsoar_Flight_setQNH(Pyxcsoar_Flight *self, PyObject *args);
PyObject* xcsoar_Flight_path(Pyxcsoar_Flight *self, PyObject *args);
PyObject* xcsoar_Flight_times(Pyxcsoar_Flight *self);
PyObject* xcsoar_Flight_reduce(Pyxcsoar_Flight *self, PyObject *args, PyObject *kwargs);
PyObject* xcsoar_Flight_analyse(Pyxcsoar_Flight *self, PyObject *args, PyObject *kwargs);
PyObject* xcsoar_Flight_encode(Pyxcsoar_Flight *self, PyObject *args);

bool Flight_init(PyObject* m);
