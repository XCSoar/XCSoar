// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The XCSoar Project

#pragma once

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
