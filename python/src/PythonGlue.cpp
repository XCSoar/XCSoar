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
#include "PythonConverters.hpp"
#include "Flight/Flight.hpp"
#include "Time/BrokenDateTime.hpp"
#include "Flight/IGCFixEnhanced.hpp"

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

PyObject* xcsoar_Flight_path(Pyxcsoar_Flight *self, PyObject *args) {
  PyObject *py_begin = NULL,
           *py_end = NULL;

  if (!PyArg_ParseTuple(args, "|OO", &py_begin, &py_end)) {
    PyErr_SetString(PyExc_AttributeError, "Can't parse argument list.");
    Py_RETURN_NONE;
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
    fix.Apply(basic, replay->Calculated());

    PyObject *py_fix_datetime = Python::BrokenDateTimeToPy(basic.date_time_utc);
    PyObject *py_fix_time = PyInt_FromLong(basic.time);
    PyObject *py_fix_location = Python::WriteLonLat(fix.location);
    PyObject *py_fix_gps_altitude = PyInt_FromLong(fix.gps_altitude);
    PyObject *py_fix_pressure_altitude = PyInt_FromLong(fix.pressure_altitude);
    PyObject *py_fix_engine_noise_level = PyInt_FromLong(fix.enl);
    PyObject *py_fix_track = PyInt_FromLong(fix.trt);
    PyObject *py_fix_ground_speed = PyInt_FromLong(fix.gsp);
    PyObject *py_fix_tas = PyInt_FromLong(fix.tas);
    PyObject *py_fix_ias = PyInt_FromLong(fix.ias);
    PyObject *py_fix_satellites = PyInt_FromLong(fix.siu);
    PyObject *py_fix_level = PyInt_FromLong(replay->Level());

    PyObject *py_fix = PyTuple_Pack(12,
      py_fix_datetime,
      py_fix_time,
      py_fix_location,
      py_fix_gps_altitude,
      py_fix_pressure_altitude,
      py_fix_engine_noise_level,
      py_fix_track,
      py_fix_ground_speed,
      py_fix_tas,
      py_fix_ias,
      py_fix_satellites,
      py_fix_level);

    if (PyList_Append(py_fixes, py_fix))
      Py_RETURN_NONE;

    Py_DECREF(py_fix);

    Py_DECREF(py_fix_time);
    Py_DECREF(py_fix_datetime);
    Py_DECREF(py_fix_location);
    Py_DECREF(py_fix_gps_altitude);
    Py_DECREF(py_fix_pressure_altitude);
    Py_DECREF(py_fix_engine_noise_level);
    Py_DECREF(py_fix_track);
    Py_DECREF(py_fix_ground_speed);
    Py_DECREF(py_fix_tas);
    Py_DECREF(py_fix_ias);
    Py_DECREF(py_fix_satellites);
    Py_DECREF(py_fix_level);
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
    PyObject *py_single_flight = PyDict_New();

    PyObject *py_takeoff = Python::WriteEvent(
      times.takeoff_time,
      times.takeoff_location);

    PyObject *py_release = Python::WriteEvent(
      times.release_time,
      times.release_location);

    PyObject *py_landing = Python::WriteEvent(
      times.landing_time,
      times.landing_location);

    PyDict_SetItemString(py_single_flight, "takeoff", py_takeoff);
    PyDict_SetItemString(py_single_flight, "release", py_release);
    PyDict_SetItemString(py_single_flight, "landing", py_landing);

    Py_DECREF(py_takeoff);
    Py_DECREF(py_release);
    Py_DECREF(py_landing);

    if (PyList_Append(py_times, py_single_flight))
      Py_RETURN_NONE;

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
    Py_RETURN_NONE;
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

  Py_BEGIN_ALLOW_THREADS
  self->flight->Reduce(begin, end, num_levels,
    zoom_factor, threshold, force_endpoints, max_delta_time, max_points);
  Py_END_ALLOW_THREADS

  Py_RETURN_NONE;
}

PyObject* xcsoar_Flight_analyse(Pyxcsoar_Flight *self, PyObject *args, PyObject *kwargs) {
  static char *kwlist[] = {"takeoff", "release", "landing",
                           "full", "triangle", "sprint",
                           "max_iterations", "max_tree_size", NULL};
  PyObject *py_takeoff, *py_release, *py_landing;
  unsigned full = 512,
           triangle = 1024,
           sprint = 96,
           max_iterations = 20e6,
           max_tree_size = 5e6;

  if (!PyArg_ParseTupleAndKeywords(args, kwargs, "OOO|IIIII", kwlist,
                                   &py_takeoff, &py_release, &py_landing,
                                   &full, &triangle, &sprint,
                                   &max_iterations, &max_tree_size)) {
    PyErr_SetString(PyExc_AttributeError, "Can't parse argument list.");
    Py_RETURN_NONE;
  }

  if (!PyDateTime_Check(py_takeoff) || !PyDateTime_Check(py_release) || !PyDateTime_Check(py_landing)) {
    PyErr_SetString(PyExc_TypeError, "Expected a DateTime object for takeoff, release and landing.");
    Py_RETURN_NONE;
  }

  BrokenDateTime takeoff = Python::PyToBrokenDateTime(py_takeoff);
  BrokenDateTime release = Python::PyToBrokenDateTime(py_release);
  BrokenDateTime landing = Python::PyToBrokenDateTime(py_landing);

  ContestStatistics olc_plus;
  ContestStatistics dmst;

  PhaseList phase_list;
  PhaseTotals phase_totals;

  Py_BEGIN_ALLOW_THREADS
  self->flight->Analyse(takeoff, release, landing,
    olc_plus, dmst,
    phase_list, phase_totals,
    full, triangle, sprint,
    max_iterations, max_tree_size);
  Py_END_ALLOW_THREADS

  PyObject *py_result = PyDict_New();

  /* write contests */
  PyObject *py_contests = PyDict_New();

  /* write olc_plus statistics */
  PyObject *py_olc_plus = PyDict_New();

  PyObject *py_classic = Python::WriteContest(olc_plus.result[0], olc_plus.solution[0]);
  PyObject *py_triangle = Python::WriteContest(olc_plus.result[1], olc_plus.solution[1]);
  PyObject *py_plus = Python::WriteContest(olc_plus.result[2], olc_plus.solution[2]);

  PyDict_SetItemString(py_olc_plus, "classic", py_classic);
  PyDict_SetItemString(py_olc_plus, "triangle", py_triangle);
  PyDict_SetItemString(py_olc_plus, "plus", py_plus);

  Py_DECREF(py_classic);
  Py_DECREF(py_triangle);
  Py_DECREF(py_plus);

  PyDict_SetItemString(py_contests, "olc_plus", py_olc_plus);
  Py_DECREF(py_olc_plus);

  /* write dmst statistics */
  PyObject *py_dmst = PyDict_New();

  PyObject *py_quadrilateral = Python::WriteContest(dmst.result[0], dmst.solution[0]);
  PyDict_SetItemString(py_dmst, "quadrilateral", py_quadrilateral);
  Py_DECREF(py_quadrilateral);

  PyDict_SetItemString(py_contests, "dmst", py_dmst);
  Py_DECREF(py_dmst);

  PyDict_SetItemString(py_result, "contests", py_contests);
  Py_DECREF(py_contests);

  /* write fligh phases */
  PyObject *py_phases = PyList_New(0);

  for (Phase phase : phase_list) {
    PyObject *py_phase = Python::WritePhase(phase);
    if (PyList_Append(py_phases, py_phase))
      return NULL;

    Py_DECREF(py_phase);
  }

  PyDict_SetItemString(py_result, "phases", py_phases);
  Py_DECREF(py_phases);

  /* write performance stats */
  PyObject *py_performance = Python::WritePerformanceStats(phase_totals);
  PyDict_SetItemString(py_result, "performance", py_performance);
  Py_DECREF(py_performance);

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

  PyModule_AddObject(m, "Flight", (PyObject *)&xcsoar_Flight_Type);
}
