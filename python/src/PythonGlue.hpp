/* Copyright_License {

  XCSoar Glide Computer - http://www.xcsoar.org/
  Copyright (C) 2000-2014 The XCSoar Project
  A detailed list of copyright holders can be found in the file "AUTHORS".

  This program is free software; you can redistribute it and/or
  modify it under the terms of the GNU General Public License
  as published by the Free Software Foundation; either version 2
  of the License, or (at your option) any later version.

  This program is distributed in the hope that it will be useful,
  but WITHOUT ANY WARRANTY; without even the implied warranty of
  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
  GNU General Public License for more details.

  You should have received a copy of the GNU General Public License
  along with this program; if not, write to the Free Software
  Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA  02111-1307, USA.
}
*/

#ifndef PYTHON_PYTHONGLUE_HPP
#define PYTHON_PYTHONGLUE_HPP

#include <Python.h>
#include "Flight/Flight.hpp"

/* xcsoar.Flight methods */
struct Pyxcsoar_Flight {
  PyObject_HEAD Flight *flight;
  char *filename;
};

static PyObject* xcsoar_Flight_new(PyTypeObject *type, PyObject *args, PyObject *kwargs);
static void xcsoar_Flight_dealloc(Pyxcsoar_Flight *self);

static PyObject* xcsoar_Flight_path(Pyxcsoar_Flight *self, PyObject *args);
static PyObject* xcsoar_Flight_times(Pyxcsoar_Flight *self);
static PyObject* xcsoar_Flight_reduce(Pyxcsoar_Flight *self, PyObject *args, PyObject *kwargs);
static PyObject* xcsoar_Flight_analyse(Pyxcsoar_Flight *self, PyObject *args, PyObject *kwargs);
static PyObject* xcsoar_Flight_encode(Pyxcsoar_Flight *self, PyObject *args);

static PyMethodDef xcsoar_Flight_methods[] = {
  {"path", (PyCFunction)xcsoar_Flight_path, METH_VARARGS, "Get flight as list."},
  {"times", (PyCFunction)xcsoar_Flight_times, METH_VARARGS, "Get takeoff/release/landing times from flight."},
  {"reduce", (PyCFunction)xcsoar_Flight_reduce, METH_VARARGS | METH_KEYWORDS, "Reduce flight."},
  {"analyse", (PyCFunction)xcsoar_Flight_analyse, METH_VARARGS | METH_KEYWORDS, "Analyse flight."},
  {"encode", (PyCFunction)xcsoar_Flight_encode, METH_VARARGS, "Return encoded flight."},
  {NULL, NULL, 0, NULL}
};

static PyMemberDef xcsoar_Flight_members[] = {
  {NULL}  /* Sentinel */
};

PyTypeObject xcsoar_Flight_Type = {
  PyObject_HEAD_INIT(&PyType_Type)
  0,                     /* obj_size */
  "xcsoar",         /* char *tp_name; */
  sizeof(Pyxcsoar_Flight), /* int tp_basicsize; */
  0,                     /* int tp_itemsize; not used much */
  (destructor)xcsoar_Flight_dealloc, /* destructor tp_dealloc; */
  0,                     /* printfunc  tp_print; */
  0,                     /* getattrfunc  tp_getattr; __getattr__ */
  0,                     /* setattrfunc  tp_setattr; __setattr__ */
  0,                     /* cmpfunc  tp_compare; __cmp__ */
  0,                     /* reprfunc  tp_repr; __repr__ */
  0,                     /* PyNumberMethods *tp_as_number; */
  0,                     /* PySequenceMethods *tp_as_sequence; */
  0,                     /* PyMappingMethods *tp_as_mapping; */
  0,                     /* hashfunc tp_hash; __hash__ */
  0,                     /* ternaryfunc tp_call; __call__ */
  0,                     /* reprfunc tp_str; __str__ */
  0,                     /* tp_getattro */
  0,                     /* tp_setattro */
  0,                     /* tp_as_buffer */
  Py_TPFLAGS_DEFAULT | Py_TPFLAGS_BASETYPE, /* tp_flags */
  "xcsoar.Flight object",  /* tp_doc */
  0,		         /* tp_traverse */
  0,		         /* tp_clear */
  0,		         /* tp_richcompare */
  0,		         /* tp_weaklistoffset */
  0,		         /* tp_iter */
  0,		         /* tp_iternext */
  xcsoar_Flight_methods,   /* tp_methods */
  xcsoar_Flight_members,   /* tp_members */
  0,                     /* tp_getset */
  0,                     /* tp_base */
  0,                     /* tp_dict */
  0,                     /* tp_descr_get */
  0,                     /* tp_descr_set */
  0,                     /* tp_dictoffset */
  0,                     /* tp_init */
  0,                     /* tp_alloc */
  xcsoar_Flight_new,     /* tp_new */
/* this could be extended even further...
   * http://starship.python.net/crew/arcege/extwriting/pyext.html
   */
};

/* xcsoar methods */
static PyObject* xcsoar_encode(PyObject *self, PyObject *args, PyObject *kwargs);

static PyMethodDef xcsoar_methods[] = {
  {"encode", (PyCFunction)xcsoar_encode, METH_VARARGS | METH_KEYWORDS, "Encode a list of numbers."},
  {NULL, NULL, 0, NULL}
};

PyMODINIT_FUNC initxcsoar();

#endif /* PYTHON_PYTHONGLUE_HPP */
