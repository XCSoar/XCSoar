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
#include <datetime.h>

#include "PythonConverters.hpp"

#include "Geo/GeoPoint.hpp"
#include "Time/BrokenDateTime.hpp"
#include "Engine/Contest/ContestTrace.hpp"
#include "Engine/Contest/ContestResult.hpp"
#include "FlightPhaseDetector.hpp"

PyObject* Python::BrokenDateTimeToPy(const BrokenDateTime &datetime) {
  PyDateTime_IMPORT;

  return PyDateTime_FromDateAndTime(
    datetime.year, datetime.month, datetime.day,
    datetime.hour, datetime.minute, datetime.second,
    0);
};

BrokenDateTime Python::PyToBrokenDateTime(PyObject *py_datetime) {
  return BrokenDateTime(
    PyDateTime_GET_YEAR(py_datetime),
    PyDateTime_GET_MONTH(py_datetime),
    PyDateTime_GET_DAY(py_datetime),
    PyDateTime_DATE_GET_HOUR(py_datetime),
    PyDateTime_DATE_GET_MINUTE(py_datetime),
    PyDateTime_DATE_GET_SECOND(py_datetime));
};

PyObject* Python::WriteLonLat(const GeoPoint &location) {
  PyObject *py_location = PyDict_New();

  PyObject *py_lon = PyFloat_FromDouble(location.longitude.Degrees());
  PyObject *py_lat = PyFloat_FromDouble(location.latitude.Degrees());
  PyDict_SetItemString(py_location, "longitude", py_lon);
  PyDict_SetItemString(py_location, "latitude", py_lat);
  Py_DECREF(py_lon);
  Py_DECREF(py_lat);

  return py_location;
}

PyObject* Python::WriteEvent(const BrokenDateTime &datetime,
                             const GeoPoint &location) {
  PyObject *py_event = PyDict_New();

  if (datetime.IsPlausible()) {
    PyObject *py_datetime = BrokenDateTimeToPy(datetime);
    PyDict_SetItemString(py_event, "time", py_datetime);
    Py_DECREF(py_datetime);
  }

  if (location.IsValid()) {
    PyObject *py_location = WriteLonLat(location);
    PyDict_Merge(py_event, py_location, true);
    Py_DECREF(py_location);
  }

  return py_event;
}

PyObject* Python::WritePoint(const ContestTracePoint &point,
                             const ContestTracePoint *previous) {
  PyObject *py_point = PyDict_New();

  PyObject *py_time = PyInt_FromLong((long)point.GetTime());
  PyObject *py_location = WriteLonLat(point.GetLocation());

  PyDict_SetItemString(py_point, "time", py_time);
  PyDict_SetItemString(py_point, "location", py_location);

  Py_DECREF(py_time);
  Py_DECREF(py_location);

  if (previous != nullptr) {
    double distance = point.DistanceTo(previous->GetLocation());
    PyObject *py_distance = PyFloat_FromDouble(distance);
    PyDict_SetItemString(py_point, "distance", py_distance);
    Py_DECREF(py_distance);

    unsigned duration =
      std::max((int)point.GetTime() - (int)previous->GetTime(), 0);
    PyObject *py_duration = PyFloat_FromDouble(duration);
    PyDict_SetItemString(py_point, "duration", py_duration);
    Py_DECREF(py_duration);

    if (duration > 0) {
      double speed = distance / duration;
      PyObject *py_speed = PyFloat_FromDouble(speed);
      PyDict_SetItemString(py_point, "speed", py_speed);
      Py_DECREF(py_speed);
    }
  }

  return py_point;
}

PyObject* Python::WriteContest(const ContestResult &result,
                               const ContestTraceVector &trace) {
  PyObject *py_contest = PyDict_New();

  PyObject *py_score = PyFloat_FromDouble(result.score);
  PyDict_SetItemString(py_contest, "score", py_score);
  Py_DECREF(py_score);

  PyObject *py_distance = PyFloat_FromDouble(result.distance);
  PyDict_SetItemString(py_contest, "distance", py_distance);
  Py_DECREF(py_distance);

  PyObject *py_duration = PyInt_FromLong((long)result.time);
  PyDict_SetItemString(py_contest, "duration", py_duration);
  Py_DECREF(py_duration);

  PyObject *py_speed = PyFloat_FromDouble(result.GetSpeed());
  PyDict_SetItemString(py_contest, "speed", py_speed);
  Py_DECREF(py_speed);

  PyObject *py_trace = PyList_New(0);

  const ContestTracePoint *previous = NULL;
  for (auto i = trace.begin(), end = trace.end(); i != end; ++i) {
    PyObject *py_point = WritePoint(*i, previous);

    if (PyList_Append(py_trace, py_point))
      return NULL;

    Py_DECREF(py_point);
    previous = &*i;
  }

  PyDict_SetItemString(py_contest, "turnpoints", py_trace);
  Py_DECREF(py_trace);

  return py_contest;
}

static const char *
FormatPhaseType(Phase::Type phase_type)
{
  switch (phase_type) {
  case Phase::Type::CRUISE:
    return "cruise";
  case Phase::Type::CIRCLING:
    return "circling";
  case Phase::Type::POWERED:
    return "powered";
  default:
    return "";
  }
}

static const char *
FormatCirclingDirection(Phase::CirclingDirection circling_direction)
{
  switch (circling_direction) {
  case Phase::CirclingDirection::LEFT:
    return "left";
  case Phase::CirclingDirection::RIGHT:
    return "right";
  case Phase::CirclingDirection::MIXED:
    return "mixed";
  default:
    return "";
  }
}

PyObject* Python::WritePhase(const Phase &phase) {
  PyObject *py_phase = PyDict_New();

  PyObject *py_start = BrokenDateTimeToPy(phase.start_datetime);
  PyObject *py_end = BrokenDateTimeToPy(phase.end_datetime);

  PyDict_SetItemString(py_phase, "start_time", py_start);
  PyDict_SetItemString(py_phase, "end_time", py_end);

  Py_DECREF(py_start);
  Py_DECREF(py_end);

  PyObject *py_type = PyString_FromString(FormatPhaseType(phase.phase_type));
  PyDict_SetItemString(py_phase, "type", py_type);
  Py_DECREF(py_type);

  PyObject *py_duration = PyInt_FromLong((long)phase.duration);
  PyDict_SetItemString(py_phase, "duration", py_duration);
  Py_DECREF(py_duration);

  PyObject *py_circling_dir = PyString_FromString(
    FormatCirclingDirection(phase.circling_direction));
  PyDict_SetItemString(py_phase, "circling_direction", py_circling_dir);
  Py_DECREF(py_circling_dir);

  PyObject *py_alt_diff = PyInt_FromLong((long)phase.alt_diff);
  PyDict_SetItemString(py_phase, "alt_diff", py_alt_diff);
  Py_DECREF(py_alt_diff);

  PyObject *py_distance = PyInt_FromLong((long)phase.distance);
  PyDict_SetItemString(py_phase, "distance", py_distance);
  Py_DECREF(py_distance);

  PyObject *py_speed = PyFloat_FromDouble(phase.GetSpeed());
  PyDict_SetItemString(py_phase, "speed", py_speed);
  Py_DECREF(py_speed);

  PyObject *py_vario = PyFloat_FromDouble(phase.GetVario());
  PyDict_SetItemString(py_phase, "vario", py_vario);
  Py_DECREF(py_vario);

  PyObject *py_glide_rate = PyFloat_FromDouble(phase.GetGlideRate());
  PyDict_SetItemString(py_phase, "glide_rate", py_glide_rate);
  Py_DECREF(py_glide_rate);

  return py_phase;
}

PyObject* Python::WriteCirclingStats(const Phase &stats) {
  PyObject *py_stats = PyDict_New();

  PyObject *py_alt_diff = PyInt_FromLong((long)stats.alt_diff);
  PyDict_SetItemString(py_stats, "alt_diff", py_alt_diff);
  Py_DECREF(py_alt_diff);

  PyObject *py_duration = PyInt_FromLong((long)stats.duration);
  PyDict_SetItemString(py_stats, "duration", py_duration);
  Py_DECREF(py_duration);

  PyObject *py_fraction = PyFloat_FromDouble(stats.fraction);
  PyDict_SetItemString(py_stats, "fraction", py_fraction);
  Py_DECREF(py_fraction);

  PyObject *py_vario = PyFloat_FromDouble(stats.GetVario());
  PyDict_SetItemString(py_stats, "vario", py_vario);
  Py_DECREF(py_vario);

  PyObject *py_count = PyInt_FromLong((long)stats.merges);
  PyDict_SetItemString(py_stats, "count", py_count);
  Py_DECREF(py_count);

  return py_stats;
}

PyObject* Python::WriteCruiseStats(const Phase &stats) {
  PyObject *py_stats = PyDict_New();

  PyObject *py_alt_diff = PyInt_FromLong((long)stats.alt_diff);
  PyDict_SetItemString(py_stats, "alt_diff", py_alt_diff);
  Py_DECREF(py_alt_diff);

  PyObject *py_duration = PyInt_FromLong((long)stats.duration);
  PyDict_SetItemString(py_stats, "duration", py_duration);
  Py_DECREF(py_duration);

  PyObject *py_fraction = PyFloat_FromDouble(stats.fraction);
  PyDict_SetItemString(py_stats, "fraction", py_fraction);
  Py_DECREF(py_fraction);

  PyObject *py_distance = PyInt_FromLong((long)stats.distance);
  PyDict_SetItemString(py_stats, "distance", py_distance);
  Py_DECREF(py_distance);

  PyObject *py_speed = PyFloat_FromDouble(stats.GetSpeed());
  PyDict_SetItemString(py_stats, "speed", py_speed);
  Py_DECREF(py_speed);

  PyObject *py_vario = PyFloat_FromDouble(stats.GetVario());
  PyDict_SetItemString(py_stats, "vario", py_vario);
  Py_DECREF(py_vario);

  PyObject *py_glide_rate = PyFloat_FromDouble(stats.GetGlideRate());
  PyDict_SetItemString(py_stats, "glide_rate", py_glide_rate);
  Py_DECREF(py_glide_rate);

  PyObject *py_count = PyInt_FromLong((long)stats.merges);
  PyDict_SetItemString(py_stats, "count", py_count);
  Py_DECREF(py_count);

  return py_stats;
}

PyObject* Python::WritePerformanceStats(const PhaseTotals &totals) {
  PyObject *py_totals = PyDict_New();

  PyObject *py_circling_total = WriteCirclingStats(totals.total_circstats);
  PyObject *py_circling_left = WriteCirclingStats(totals.left_circstats);
  PyObject *py_circling_right = WriteCirclingStats(totals.right_circstats);
  PyObject *py_circling_mixed = WriteCirclingStats(totals.mixed_circstats);
  PyObject *py_cruise_total = WriteCruiseStats(totals.total_cruisestats);

  PyDict_SetItemString(py_totals, "circling_total", py_circling_total);
  PyDict_SetItemString(py_totals, "circling_left", py_circling_left);
  PyDict_SetItemString(py_totals, "circling_right", py_circling_right);
  PyDict_SetItemString(py_totals, "circling_mixed", py_circling_mixed);
  PyDict_SetItemString(py_totals, "cruise_total", py_cruise_total);

  Py_DECREF(py_circling_total);
  Py_DECREF(py_circling_left);
  Py_DECREF(py_circling_right);
  Py_DECREF(py_circling_mixed);
  Py_DECREF(py_cruise_total);

  return py_totals;
}

