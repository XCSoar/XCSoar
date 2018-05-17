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

#ifndef PYTHON_AIRSPACES_HPP
#define PYTHON_AIRSPACES_HPP

#include <Python.h>

#include "Engine/Airspace/Airspaces.hpp"
#include "Engine/Airspace/AirspaceClass.hpp"

/* xcsoar.Airspaces methods */
struct Pyxcsoar_Airspaces {
  PyObject_HEAD Airspaces *airspace_database;
};

struct AirspaceClassStringCouple
{
  const char *string;
  AirspaceClass type;
};

PyObject* xcsoar_Airspaces_new(PyTypeObject *type, PyObject *args, PyObject *kwargs);
void xcsoar_Airspaces_dealloc(Pyxcsoar_Airspaces *self);

PyObject* xcsoar_Airspaces_addPolygon(Pyxcsoar_Airspaces *self, PyObject *args);
PyObject* xcsoar_Airspaces_optimise(Pyxcsoar_Airspaces *self);
PyObject* xcsoar_Airspaces_findIntrusions(Pyxcsoar_Airspaces *self, PyObject *args);

bool Airspaces_init(PyObject* m);

#endif /* PYTHON_AIRSPACES_HPP */
