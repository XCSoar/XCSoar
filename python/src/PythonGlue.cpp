/* Copyright_License {

  XCSoar Glide Computer - http://www.xcsoar.org/
  Copyright (C) 2000-2013 The XCSoar Project
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

#include <Python.h>
#include <structmember.h> /* required for PyMemberDef */
#include <datetime.h>

#include "PythonGlue.hpp"
#include "Flight.hpp"

#include <cstdio>
#include <vector>
#include <cinttypes>
#include <limits>

Pyxcsoar_Flight* xcsoar_Flight_init(Pyxcsoar_Flight *self, PyObject *args, PyObject *kwargs) {
  /* constructor */
  static char *kwlist[] = {"file", "keep", NULL};
  const char *input_file;
  bool keep = false;

  if (!PyArg_ParseTupleAndKeywords(args, kwargs, "s|b", kwlist,
                                   &input_file, &keep)) {
    PyErr_SetString(PyExc_AttributeError, "Can't parse argument list.");
    return 0;
  }

  Py_BEGIN_ALLOW_THREADS
  self->flight = new Flight(input_file, keep);
  Py_END_ALLOW_THREADS

  return 0;
}

void xcsoar_Flight_dealloc(Pyxcsoar_Flight *self) {
  /* destructor */
  delete self->flight;
}

PyMODINIT_FUNC
__attribute__ ((visibility("default")))
initxcsoar() {
  PyObject* m;

  if (PyType_Ready(&xcsoar_Flight_Type) < 0)
      return;

  m = Py_InitModule3("xcsoar", xcsoar_methods, "XCSoar Tools");

  if (m == NULL)
    return;

  PyDateTime_IMPORT;

  Py_INCREF(&xcsoar_Flight_Type);
  PyModule_AddObject(m, "Flight", (PyObject *)&xcsoar_Flight_Type);
}
