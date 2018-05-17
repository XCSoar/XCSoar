/* Copyright_License {

  XCSoar Glide Computer - http://www.xcsoar.org/
  Copyright (C) 2000-2016 The XCSoar Project
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

#ifndef PYTHON_FLIGHT_HPP
#define PYTHON_FLIGHT_HPP

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

#endif /* PYTHON_FLIGHT_HPP */
