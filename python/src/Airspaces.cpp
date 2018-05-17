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

#include <Python.h>
#include <structmember.h> /* required for PyMemberDef */

#include "Airspaces.hpp"
#include "Flight.hpp"

#include "PythonConverters.hpp"

#include "Engine/Airspace/Airspaces.hpp"
#include "Engine/Airspace/AirspacePolygon.hpp"
#include "Engine/Airspace/AbstractAirspace.hpp"
#include "Engine/Airspace/AirspaceClass.hpp"
#include "Engine/Airspace/AirspaceAltitude.hpp"
#include "Engine/Navigation/Aircraft.hpp"
#include "NMEA/Aircraft.hpp"
#include "Util/tstring.hpp"
#include "Util/Macros.hpp"

#include <vector>
#include <algorithm>

static constexpr AirspaceClassStringCouple airspace_class_strings[] = {
  { "CLASSA", CLASSA },
  { "CLASSB", CLASSB },
  { "CLASSC", CLASSC },
  { "CLASSD", CLASSD },
  { "CLASSE", CLASSE },
  { "CLASSF", CLASSF },
  { "CLASSG", CLASSG },
  { "CTR", CTR },
  { "TMZ", TMZ },
  { "RESTRICT", RESTRICT },
  { "PROHIBITED", PROHIBITED },
  { "DANGER", DANGER },
  { "NOGLIDER", NOGLIDER },
  { "WAVE", WAVE },
  { "MATZ", MATZ },
  { "AATASK", AATASK },
  { "OTHER", OTHER },
  { "RMZ", RMZ },
};

PyObject* xcsoar_Airspaces_new(PyTypeObject *type, PyObject *args, PyObject *kwargs) {
  /* constructor */
  Pyxcsoar_Airspaces *self;
  self = (Pyxcsoar_Airspaces *)type->tp_alloc(type, 0);

  self->airspace_database = new Airspaces();

  return (PyObject*) self;
}

void xcsoar_Airspaces_dealloc(Pyxcsoar_Airspaces *self) {
  /* destructor */
  delete self->airspace_database;

  self->ob_type->tp_free((Pyxcsoar_Airspaces*)self);
}

PyObject* xcsoar_Airspaces_addPolygon(Pyxcsoar_Airspaces *self, PyObject *args) {
  PyObject *py_points = nullptr,
           *py_name = nullptr,
           *py_as_class = nullptr,
           *py_base_ref = nullptr,
           *py_top_ref = nullptr;
  double base_alt, top_alt;

  if (!PyArg_ParseTuple(args, "OOOdOdO", &py_points, &py_name, &py_as_class,
                                         &base_alt, &py_base_ref,
                                         &top_alt, &py_top_ref)) {
    PyErr_SetString(PyExc_AttributeError, "Error reading attributes.");
    return nullptr;
  }

  /* Parse points */
  std::vector<GeoPoint> points;

  if (!PySequence_Check(py_points)) {
    PyErr_SetString(PyExc_ValueError, "First argument is no sequence");
    return nullptr;
  }

  Py_ssize_t num_items = PySequence_Fast_GET_SIZE(py_points);

  for (Py_ssize_t i = 0; i < num_items; ++i) {
    PyObject *py_location = PySequence_Fast_GET_ITEM(py_points, i);

    GeoPoint location = Python::ReadLonLat(py_location);

    if (!location.IsValid()) {
      if (PyErr_Occurred() == nullptr)
        PyErr_SetString(PyExc_RuntimeError, "Unknown error while parsing location");

      return nullptr;
    }

    points.push_back(location);
  }

  if (points.size() < 3) {
    PyErr_SetString(PyExc_ValueError, "Polygon has not enough points");
    return nullptr;
  }

  /* Parse airspace name */
  tstring name;

  if (!Python::PyStringToString(py_name, name)) {
    PyErr_SetString(PyExc_ValueError, "Can't parse airspace name.");
    return nullptr;
  }

  /* Parse airspace class */
  tstring as_class;
  AirspaceClass type = AirspaceClass::OTHER;

  if (!Python::PyStringToString(py_as_class, as_class)) {
    PyErr_SetString(PyExc_ValueError, "Can't parse airspace class.");
    return nullptr;
  }

  for (unsigned i = 0; i < ARRAY_SIZE(airspace_class_strings); i++) {
    if (as_class.compare(airspace_class_strings[i].string) == 0)
      type = airspace_class_strings[i].type;
  }

  /* Parse airspace base and top */
  tstring base_ref, top_ref;
  AirspaceAltitude base, top;

  if (!Python::PyStringToString(py_base_ref, base_ref)) {
    PyErr_SetString(PyExc_ValueError, "Can't parse airspace base reference.");
    return nullptr;
  }

  if (!Python::PyStringToString(py_top_ref, top_ref)) {
    PyErr_SetString(PyExc_ValueError, "Can't parse airspace top reference.");
    return nullptr;
  }

  if (base_ref.compare("MSL") == 0) {
    base.reference = AltitudeReference::MSL;
    base.altitude = base_alt;
  } else if (base_ref.compare("FL") == 0) {
    base.reference = AltitudeReference::STD;
    base.flight_level = base_alt;
  } else if (base_ref.compare("AGL") == 0) {
    base.reference = AltitudeReference::AGL;
    base.altitude_above_terrain = base_alt;
  } else {
    PyErr_SetString(PyExc_ValueError, "Can't parse airspace base.");
    return nullptr;
  }

  if (top_ref.compare("MSL") == 0) {
    top.reference = AltitudeReference::MSL;
    top.altitude = top_alt;
  } else if (top_ref.compare("FL") == 0) {
    top.reference = AltitudeReference::STD;
    top.flight_level = top_alt;
  } else if (top_ref.compare("AGL") == 0) {
    top.reference = AltitudeReference::AGL;
    top.altitude_above_terrain = top_alt;
  } else {
    PyErr_SetString(PyExc_ValueError, "Can't parse airspace top.");
    return nullptr;
  }

  /* Create airspace and save it into the database */
  AbstractAirspace *as = new AirspacePolygon(points);
  as->SetProperties(std::move(name), type, base, top);
  self->airspace_database->Add(as);

  Py_RETURN_NONE;
}

PyObject* xcsoar_Airspaces_optimise(Pyxcsoar_Airspaces *self) {
  self->airspace_database->Optimise();

  Py_RETURN_NONE;
}

PyObject* xcsoar_Airspaces_findIntrusions(Pyxcsoar_Airspaces *self, PyObject *args) {
  PyObject *py_flight = nullptr;

  if (!PyArg_ParseTuple(args, "O", &py_flight)) {
    PyErr_SetString(PyExc_AttributeError, "Can't parse argument.");
    return nullptr;
  }

  DebugReplay *replay = ((Pyxcsoar_Flight*)py_flight)->flight->Replay();

  if (replay == nullptr) {
    PyErr_SetString(PyExc_IOError, "Can't start replay - file not found.");
    return nullptr;
  }

  PyObject *py_result = PyDict_New();
  Airspaces::AirspaceVector last_airspaces;

  while (replay->Next()) {
    const MoreData &basic = replay->Basic();

    if (!basic.time_available || !basic.location_available ||
        !basic.NavAltitudeAvailable())
      continue;

    const auto range =
      self->airspace_database->QueryInside(ToAircraftState(basic,
                                                           replay->Calculated()));
    Airspaces::AirspaceVector airspaces(range.begin(), range.end());
    for (auto it = airspaces.begin(); it != airspaces.end(); it++) {
      PyObject *py_name = PyString_FromString((*it).GetAirspace().GetName());
      PyObject *py_airspace = nullptr,
               *py_period = nullptr;

      if (PyDict_Contains(py_result, py_name) == 0) {
        // this is the first fix inside this airspace
        py_airspace = PyList_New(0);
        PyDict_SetItem(py_result, py_name, py_airspace);

        py_period = PyList_New(0);
        PyList_Append(py_airspace, py_period);
        Py_DECREF(py_period);

      } else {
        // this airspace was hit some time before...
        py_airspace = PyDict_GetItem(py_result, py_name);

        // check if the last fix was already inside this airspace
        auto in_last = std::find(last_airspaces.begin(), last_airspaces.end(), *it);

        if (in_last == last_airspaces.end()) {
          // create a new period
          py_period = PyList_New(0);
          PyList_Append(py_airspace, py_period);
          Py_DECREF(py_period);
        } else {
          py_period = PyList_GET_ITEM(py_airspace, PyList_GET_SIZE(py_airspace) - 1);
        }
      }

      PyList_Append(py_period, Py_BuildValue("{s:N,s:N}",
        "time", Python::BrokenDateTimeToPy(basic.date_time_utc),
        "location", Python::WriteLonLat(basic.location)));
    }

    last_airspaces = std::move(airspaces);
  }

  delete replay;

  return py_result;
}

PyMethodDef xcsoar_Airspaces_methods[] = {
  {"addPolygon", (PyCFunction)xcsoar_Airspaces_addPolygon, METH_VARARGS, "Add a airspace polygon."},
  {"optimise", (PyCFunction)xcsoar_Airspaces_optimise, METH_NOARGS, "Optimise airspace database."},
  {"findIntrusions", (PyCFunction)xcsoar_Airspaces_findIntrusions, METH_VARARGS, "Check flight for airspace intrusions."},
  {nullptr, nullptr, 0, nullptr}
};

PyMemberDef xcsoar_Airspaces_members[] = {
  {nullptr}  /* Sentinel */
};

PyTypeObject xcsoar_Airspaces_Type = {
  PyObject_HEAD_INIT(&PyType_Type)
  0,                     /* obj_size */
  "xcsoar",         /* char *tp_name; */
  sizeof(Pyxcsoar_Airspaces), /* int tp_basicsize; */
  0,                     /* int tp_itemsize; not used much */
  (destructor)xcsoar_Airspaces_dealloc, /* destructor tp_dealloc; */
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
  "xcsoar.Airspaces object",  /* tp_doc */
  0,                     /* tp_traverse */
  0,                     /* tp_clear */
  0,                     /* tp_richcompare */
  0,                     /* tp_weaklistoffset */
  0,                     /* tp_iter */
  0,                     /* tp_iternext */
  xcsoar_Airspaces_methods,   /* tp_methods */
  xcsoar_Airspaces_members,   /* tp_members */
  0,                     /* tp_getset */
  0,                     /* tp_base */
  0,                     /* tp_dict */
  0,                     /* tp_descr_get */
  0,                     /* tp_descr_set */
  0,                     /* tp_dictoffset */
  0,                     /* tp_init */
  0,                     /* tp_alloc */
  xcsoar_Airspaces_new,     /* tp_new */
/* this could be extended even further...
   * http://starship.python.net/crew/arcege/extwriting/pyext.html
   */
};

bool Airspaces_init(PyObject* m) {
  if (PyType_Ready(&xcsoar_Airspaces_Type) < 0)
      return false;

  Py_INCREF(&xcsoar_Airspaces_Type);
  PyModule_AddObject(m, "Airspaces", (PyObject *)&xcsoar_Airspaces_Type);

  return true;
}
