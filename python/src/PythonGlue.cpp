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

#include <Python.h>
#include <structmember.h> /* required for PyMemberDef */
#include <datetime.h>

#include "PythonGlue.hpp"
#include "PythonConverters.hpp"
#include "Flight/Flight.hpp"
#include "Time/BrokenDateTime.hpp"
#include "Flight/IGCFixEnhanced.hpp"
#include "Tools/GoogleEncode.hpp"

#include <cstdio>
#include <vector>
#include <cinttypes>
#include <limits>

PyObject* xcsoar_Flight_new(PyTypeObject *type, PyObject *args, PyObject *kwargs) {
  /* constructor */
  static char *kwlist[] = {"file", "keep", NULL};
  PyObject *py_input_data = NULL;
  bool keep = false;

  if (!PyArg_ParseTupleAndKeywords(args, kwargs, "O|b", kwlist,
                                   &py_input_data, &keep)) {
    PyErr_SetString(PyExc_AttributeError, "Can't parse argument list.");
    return 0;
  }

  Pyxcsoar_Flight *self;
  self = (Pyxcsoar_Flight *)type->tp_alloc(type, 0);
  self->filename = nullptr;

  if (PyString_Check(py_input_data) || PyUnicode_Check(py_input_data)) {
    Py_ssize_t length = PyString_Size(py_input_data);
    // add one char for \0
    self->filename = new char[length + 1];
    strncpy(self->filename, PyString_AsString(py_input_data), length + 1);

    Py_BEGIN_ALLOW_THREADS
    self->flight = new Flight(self->filename, keep);
    Py_END_ALLOW_THREADS
  } else if (PySequence_Check(py_input_data) == 1) {
    Py_ssize_t num_items = PySequence_Fast_GET_SIZE(py_input_data);

    self->flight = new Flight();

    for (Py_ssize_t i = 0; i < num_items; ++i) {
      PyObject *py_item = PySequence_Fast_GET_ITEM(py_input_data, i);

      if (PySequence_Fast_GET_SIZE(py_item) < 4) {
        PyErr_SetString(PyExc_TypeError, "Expected a tuple of at least 4.");
        return 0;
      }

      IGCFixEnhanced fix;
      fix.Clear();

      if (!Python::PyTupleToIGCFixEnhanced(py_item, fix)) {
        return 0;
      }

      self->flight->AppendFix(fix);
    }
  }

  return (PyObject*) self;
}

void xcsoar_Flight_dealloc(Pyxcsoar_Flight *self) {
  /* destructor */
  if (self->filename != nullptr)
    delete[] self->filename;

  delete self->flight;
  self->ob_type->tp_free((Pyxcsoar_Flight*)self);
}

PyObject* xcsoar_Flight_path(Pyxcsoar_Flight *self, PyObject *args) {
  PyObject *py_begin = NULL,
           *py_end = NULL;

  if (!PyArg_ParseTuple(args, "|OO", &py_begin, &py_end)) {
    PyErr_SetString(PyExc_AttributeError, "Can't parse argument list.");
    return NULL;
  }

  int64_t begin = 0,
          end = std::numeric_limits<int64_t>::max();

  if (py_begin != NULL && PyDateTime_Check(py_begin))
    begin = Python::PyToBrokenDateTime(py_begin).ToUnixTimeUTC();

  if (py_end != NULL && PyDateTime_Check(py_end))
    end = Python::PyToBrokenDateTime(py_end).ToUnixTimeUTC();

  // prepare output
  PyObject *py_fixes = PyList_New(0);

  DebugReplay *replay = self->flight->Replay();

  if (replay == nullptr) {
    PyErr_SetString(PyExc_IOError, "Can't start replay - file not found.");
    return nullptr;
  }

  while (replay->Next()) {
    if (replay->Level() == -1) continue;

    const MoreData &basic = replay->Basic();
    const int64_t date_time_utc = basic.date_time_utc.ToUnixTimeUTC();

    if (date_time_utc < begin)
      continue;
    else if (date_time_utc > end)
      break;

    if (!basic.time_available || !basic.location_available ||
        !basic.NavAltitudeAvailable())
      continue;

    IGCFixEnhanced fix;
    fix.Clear();
    fix.Apply(basic, replay->Calculated());
    fix.level = replay->Level();

    PyObject *py_fix = Python::IGCFixEnhancedToPyTuple(fix);

    if (PyList_Append(py_fixes, py_fix) != 0) {
      delete replay;
      return NULL;
    }

    Py_DECREF(py_fix);
  }

  delete replay;

  return py_fixes;
}

PyObject* xcsoar_Flight_times(Pyxcsoar_Flight *self) {
  std::vector<FlightTimeResult> results;

  Py_BEGIN_ALLOW_THREADS
  self->flight->Times(results);
  Py_END_ALLOW_THREADS

  PyObject *py_times = PyList_New(0);

  for (auto times : results) {
    PyObject *py_power_states = PyList_New(0);

    for (auto power_state : times.power_states) {
      PyObject *py_power_state = Py_BuildValue("{s:N,s:N,s:O}",
        "time", Python::BrokenDateTimeToPy(power_state.time),
        "location", Python::WriteLonLat(power_state.location),
        "powered", power_state.state == PowerState::ON ? Py_True : Py_False);

      if (PyList_Append(py_power_states, py_power_state) != 0)
        return NULL;

      Py_DECREF(py_power_state);
    }

    PyObject *py_single_flight = Py_BuildValue("{s:N,s:N,s:N}",
      "takeoff", Python::WriteEvent(times.takeoff_time, times.takeoff_location),
      "landing", Python::WriteEvent(times.landing_time, times.landing_location),
      "power_states", py_power_states);

    if (times.release_time.IsPlausible()) {
      PyObject *py_release = Python::WriteEvent(times.release_time, times.release_location);
      PyDict_SetItemString(py_single_flight, "release", py_release);
      Py_DECREF(py_release);
    }

    if (PyList_Append(py_times, py_single_flight) != 0)
      return NULL;

    Py_DECREF(py_single_flight);
  }

  return py_times;
}

PyObject* xcsoar_Flight_reduce(Pyxcsoar_Flight *self, PyObject *args, PyObject *kwargs) {
  PyObject *py_begin = NULL,
           *py_end = NULL,
           *py_force_endpoints = NULL;

  static char *kwlist[] = {"begin", "end", "num_levels", "zoom_factor",
                           "max_delta_time", "threshold", "max_points",
                           "force_endpoints", NULL};

  /* default values */
  unsigned num_levels = 4,
           zoom_factor = 4,
           max_delta_time = 30,
           max_points = std::numeric_limits<unsigned>::max();
  double threshold = 0.001;
  bool force_endpoints = true;

  if (!PyArg_ParseTupleAndKeywords(args, kwargs, "|OOIIIdIO", kwlist,
                                   &py_begin, &py_end, &num_levels, &zoom_factor,
                                   &max_delta_time, &threshold, &max_points, &py_force_endpoints)) {
    PyErr_SetString(PyExc_AttributeError, "Can't parse argument list.");
    return NULL;
  }

  if (py_force_endpoints != NULL && !PyObject_IsTrue(py_force_endpoints))
    force_endpoints = false;

  BrokenDateTime begin, end;

  if (py_begin != NULL && PyDateTime_Check(py_begin))
    begin = Python::PyToBrokenDateTime(py_begin);
  else
    begin = BrokenDateTime::FromUnixTimeUTC(0);

  if (py_end != NULL && PyDateTime_Check(py_end))
    end = Python::PyToBrokenDateTime(py_end);
  else
    /* numeric_limits<int64_t>::max() doesn't work here, because
       that's an invalid date in BrokenDate's eyes. 1970 + 2^33 secs
       is about the year 2242, which is far enough in the future :-) */
    end = BrokenDateTime::FromUnixTimeUTC(int64_t(2)<<32);

  if (end - begin < 0) {
    PyErr_SetString(PyExc_ValueError, "Start time later then end time.");
    return NULL;
  }

  Py_BEGIN_ALLOW_THREADS
  self->flight->Reduce(begin, end, num_levels,
    zoom_factor, threshold, force_endpoints, max_delta_time, max_points);
  Py_END_ALLOW_THREADS

  Py_RETURN_NONE;
}

PyObject* xcsoar_Flight_analyse(Pyxcsoar_Flight *self, PyObject *args, PyObject *kwargs) {
  static char *kwlist[] = {"takeoff", "scoring_start", "scoring_end", "landing",
                           "full", "triangle", "sprint",
                           "max_iterations", "max_tree_size", NULL};
  PyObject *py_takeoff, *py_scoring_start, *py_scoring_end, *py_landing;
  unsigned full = 512,
           triangle = 1024,
           sprint = 96,
           max_iterations = 20e6,
           max_tree_size = 5e6;

  if (!PyArg_ParseTupleAndKeywords(args, kwargs, "OOOO|IIIII", kwlist,
                                   &py_takeoff, &py_scoring_start, &py_scoring_end, &py_landing,
                                   &full, &triangle, &sprint,
                                   &max_iterations, &max_tree_size)) {
    PyErr_SetString(PyExc_AttributeError, "Can't parse argument list.");
    return NULL;
  }

  if (!PyDateTime_Check(py_takeoff) || !PyDateTime_Check(py_landing)) {
    PyErr_SetString(PyExc_TypeError, "Expected a DateTime object for takeoff and landing.");
    return NULL;
  }

  BrokenDateTime takeoff = Python::PyToBrokenDateTime(py_takeoff);
  BrokenDateTime landing = Python::PyToBrokenDateTime(py_landing);

  BrokenDateTime scoring_start, scoring_end;

  if (PyDateTime_Check(py_scoring_start))
    scoring_start = Python::PyToBrokenDateTime(py_scoring_start);

  if (PyDateTime_Check(py_scoring_end))
    scoring_end = Python::PyToBrokenDateTime(py_scoring_end);

  ContestStatistics olc_plus;
  ContestStatistics dmst;

  PhaseList phase_list;
  PhaseTotals phase_totals;

  WindList wind_list;

  bool success;

  Py_BEGIN_ALLOW_THREADS
  success = self->flight->Analyse(takeoff, scoring_start, scoring_end, landing,
    olc_plus, dmst,
    phase_list, phase_totals, wind_list,
    full, triangle, sprint,
    max_iterations, max_tree_size);
  Py_END_ALLOW_THREADS

  if (!success)
    Py_RETURN_NONE;

  /* write olc_plus statistics */
  PyObject *py_olc_plus = Py_BuildValue("{s:N,s:N,s:N}",
    "classic", Python::WriteContest(olc_plus.result[0], olc_plus.solution[0]),
    "triangle", Python::WriteContest(olc_plus.result[1], olc_plus.solution[1]),
    "plus", Python::WriteContest(olc_plus.result[2], olc_plus.solution[2]));

  /* write dmst statistics */
  PyObject *py_dmst = Py_BuildValue("{s:N}",
    "quadrilateral", Python::WriteContest(dmst.result[0], dmst.solution[0]));

  /* write contests */
  PyObject *py_contests = Py_BuildValue("{s:N,s:N}",
    "olc_plus", py_olc_plus,
    "dmst", py_dmst);

  /* write fligh phases */
  PyObject *py_phases = PyList_New(0);

  for (Phase phase : phase_list) {
    PyObject *py_phase = Python::WritePhase(phase);
    if (PyList_Append(py_phases, py_phase) != 0)
      return NULL;

    Py_DECREF(py_phase);
  }

  /* write wind list*/
  PyObject *py_wind_list = PyList_New(0);

  for (WindListItem wind_item: wind_list) {
    PyObject *py_wind = Python::WriteWindItem(wind_item);
    if (PyList_Append(py_wind_list, py_wind) != 0)
      return NULL;

    Py_DECREF(py_wind);
  }

  PyObject *py_result = Py_BuildValue("{s:N,s:N,s:N,s:N}",
    "contests", py_contests,
    "phases", py_phases,
    "performance", Python::WritePerformanceStats(phase_totals),
    "wind", py_wind_list);

  return py_result;
}

PyObject* xcsoar_Flight_encode(Pyxcsoar_Flight *self, PyObject *args) {
  PyObject *py_begin = NULL,
           *py_end = NULL;

  if (!PyArg_ParseTuple(args, "|OO", &py_begin, &py_end)) {
    PyErr_SetString(PyExc_AttributeError, "Can't parse argument list.");
    return NULL;
  }

  int64_t begin = 0,
          end = std::numeric_limits<int64_t>::max();

  if (py_begin != NULL && PyDateTime_Check(py_begin))
    begin = Python::PyToBrokenDateTime(py_begin).ToUnixTimeUTC();

  if (py_end != NULL && PyDateTime_Check(py_end))
    end = Python::PyToBrokenDateTime(py_end).ToUnixTimeUTC();

  GoogleEncode encoded_locations(2, true, 1e5),
               encoded_levels,
               encoded_times,
               encoded_altitude,
               encoded_enl;

  DebugReplay *replay = self->flight->Replay();

  if (replay == nullptr) {
    PyErr_SetString(PyExc_IOError, "Can't start replay - file not found.");
    return nullptr;
  }

  while (replay->Next()) {
    if (replay->Level() == -1) continue;

    const MoreData &basic = replay->Basic();
    const int64_t date_time_utc = basic.date_time_utc.ToUnixTimeUTC();

    if (date_time_utc < begin)
      continue;
    else if (date_time_utc > end)
      break;

    if (!basic.time_available || !basic.location_available ||
        !basic.NavAltitudeAvailable())
      continue;

    IGCFixEnhanced fix;
    fix.Clear();
    fix.Apply(basic, replay->Calculated());

    encoded_locations.addDouble(fix.location.latitude.Degrees());
    encoded_locations.addDouble(fix.location.longitude.Degrees());

    encoded_levels.addUnsignedNumber(replay->Level());
    encoded_times.addSignedNumber(basic.time);
    encoded_altitude.addSignedNumber(fix.gps_altitude);

    if (fix.enl >= 0)
        encoded_enl.addSignedNumber(fix.enl);
  }

  delete replay;


  PyObject *py_result = Py_BuildValue("{s:s,s:s,s:s,s:s,s:s}",
    "locations", encoded_locations.asString()->c_str(),
    "levels", encoded_levels.asString()->c_str(),
    "times", encoded_times.asString()->c_str(),
    "altitude", encoded_altitude.asString()->c_str(),
    "enl", encoded_enl.asString()->c_str());

  return py_result;
}


PyObject* xcsoar_encode(PyObject *self, PyObject *args, PyObject *kwargs) {
  PyObject *py_list,
           *py_method = nullptr;
  double floor_to = 1;
  bool delta = true;

  static char *kwlist[] = {"list", "delta", "floor", "method", NULL};

  if (!PyArg_ParseTupleAndKeywords(args, kwargs, "O|idO", kwlist,
                                   &py_list, &delta, &floor_to, &py_method)) {
    PyErr_SetString(PyExc_AttributeError, "Can't parse argument list.");
    return NULL;
  }

  if (!PySequence_Check(py_list)) {
    PyErr_SetString(PyExc_TypeError, "Expected a list.");
    return NULL;
  }

  Py_ssize_t num_items = PySequence_Fast_GET_SIZE(py_list);

  // return empty string if list has no elements
  if (num_items == 0)
    return PyString_FromString("");

  unsigned dimension;
  if (PySequence_Check(PySequence_Fast_GET_ITEM(py_list, 0))) {
    dimension = PySequence_Size(PySequence_Fast_GET_ITEM(py_list, 0));
  } else {
    dimension = 1;
  }

  enum Method { UNSIGNED, SIGNED, DOUBLE } method;

  if (py_method == nullptr)
    method = UNSIGNED;
  else if (PyString_Check(py_method) && strcmp(PyString_AsString(py_method), "unsigned") == 0)
    method = UNSIGNED;
  else if (PyString_Check(py_method) && strcmp(PyString_AsString(py_method), "signed") == 0)
    method = SIGNED;
  else if (PyString_Check(py_method) && strcmp(PyString_AsString(py_method), "double") == 0)
    method = DOUBLE;
  else {
    PyErr_SetString(PyExc_TypeError, "Can't parse method.");
    return NULL;
  }

  GoogleEncode encoded(dimension, delta, floor_to);

  for (Py_ssize_t i = 0; i < num_items; ++i) {
    PyObject *py_item = PySequence_Fast_GET_ITEM(py_list, i);

    if (dimension > 1) {
      for (unsigned j = 0; j < dimension; ++j) {

        if (method == UNSIGNED) {
          if (!PyNumber_Check(PySequence_Fast_GET_ITEM(py_item, j))) {
            PyErr_SetString(PyExc_TypeError, "Expected numeric value.");
            return NULL;
          }
          encoded.addUnsignedNumber(PyInt_AsLong(PySequence_Fast_GET_ITEM(py_item, j)));
        } else if (method == SIGNED) {
          if (!PyNumber_Check(PySequence_Fast_GET_ITEM(py_item, j))) {
            PyErr_SetString(PyExc_TypeError, "Expected numeric value.");
            return NULL;
          }
          encoded.addSignedNumber(PyInt_AsLong(PySequence_Fast_GET_ITEM(py_item, j)));
        } else if (method == DOUBLE) {
          if (!PyNumber_Check(PySequence_Fast_GET_ITEM(py_item, j))) {
            PyErr_SetString(PyExc_TypeError, "Expected numeric value.");
            return NULL;
          }
          encoded.addDouble(PyFloat_AsDouble(PySequence_Fast_GET_ITEM(py_item, j)));
        }

      }
    } else {

      if (method == UNSIGNED) {
        if (!PyNumber_Check(py_item)) {
          PyErr_SetString(PyExc_TypeError, "Expected numeric value.");
          return NULL;
        }
        encoded.addUnsignedNumber(PyInt_AsLong(py_item));
      } else if (method == SIGNED) {
        if (!PyNumber_Check(py_item)) {
          PyErr_SetString(PyExc_TypeError, "Expected numeric value.");
          return NULL;
        }
        encoded.addSignedNumber(PyInt_AsLong(py_item));
      } else if (method == DOUBLE) {
        if (!PyNumber_Check(py_item)) {
          PyErr_SetString(PyExc_TypeError, "Expected numeric value.");
          return NULL;
        }
        encoded.addDouble(PyFloat_AsDouble(py_item));
      }

    }
  }

  // prepare output
  PyObject *py_result = PyString_FromString(encoded.asString()->c_str());

  return py_result;
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
