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
#include <datetime.h>

#include "PythonConverters.hpp"
#include "Flight/AnalyseFlight.hpp"
#include "Flight/IGCFixEnhanced.hpp"

#include "Geo/GeoPoint.hpp"
#include "Math/Angle.hpp"
#include "Time/BrokenDateTime.hpp"
#include "Engine/Contest/ContestTrace.hpp"
#include "Engine/Contest/ContestResult.hpp"
#include "FlightPhaseDetector.hpp"

#if PY_MAJOR_VERSION >= 3
    #define PyInt_FromLong PyLong_FromLong
    #define PyInt_AsLong PyLong_AsLong
#endif

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
  return Py_BuildValue("{s:d,s:d}",
    "longitude", location.longitude.Degrees(),
    "latitude", location.latitude.Degrees());
}

GeoPoint Python::ReadLonLat(PyObject *py_location) {
  if (!PyDict_Check(py_location)) {
    PyErr_SetString(PyExc_TypeError, "Location is not a dictionary.");
    return GeoPoint::Invalid();
  }

  PyObject *py_latitude = PyDict_GetItemString(py_location, "latitude"),
           *py_longitude = PyDict_GetItemString(py_location, "longitude");

  if (!PyNumber_Check(py_latitude) || !PyNumber_Check(py_longitude)) {
    PyErr_SetString(PyExc_TypeError, "Failed to parse location.");
    return GeoPoint::Invalid();
  }

  GeoPoint location(Angle::Degrees(PyFloat_AsDouble(py_longitude)),
                    Angle::Degrees(PyFloat_AsDouble(py_latitude)));

  return location;
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
    PyDict_SetItemString(py_event, "location", py_location);
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
  PyObject *py_trace = PyList_New(0);

  const ContestTracePoint *previous = nullptr;
  for (auto i = trace.begin(), end = trace.end(); i != end; ++i) {
    PyObject *py_point = WritePoint(*i, previous);

    if (PyList_Append(py_trace, py_point))
      return nullptr;

    Py_DECREF(py_point);
    previous = &*i;
  }

  return Py_BuildValue("{s:d,s:d,s:i,s:d,s:N}",
    "score", result.score,
    "distance", result.distance,
    "duration", (long)result.time,
    "speed", result.GetSpeed(),
    "turnpoints", py_trace);
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
  return Py_BuildValue("{s:N,s:N,s:s,s:i,s:s,s:i,s:i,s:d,s:d,s:d}",
    "start_time", BrokenDateTimeToPy(phase.start_datetime),
    "end_time", BrokenDateTimeToPy(phase.end_datetime),
    "type", FormatPhaseType(phase.phase_type),
    "duration", (long)phase.duration,
    "circling_direction", FormatCirclingDirection(phase.circling_direction),
    "alt_diff", (long)phase.alt_diff,
    "distance", (long)phase.distance,
    "speed", phase.GetSpeed(),
    "vario", phase.GetVario(),
    "glide_rate", phase.GetGlideRate());
}

PyObject* Python::WriteCirclingStats(const Phase &stats) {
  return Py_BuildValue("{s:i,s:i,s:d,s:d,s:i}",
    "alt_diff", (long)stats.alt_diff,
    "duration", (long)stats.duration,
    "fraction", stats.fraction,
    "vario", stats.GetVario(),
    "count", (long)stats.merges);
}

PyObject* Python::WriteCruiseStats(const Phase &stats) {
  return Py_BuildValue("{s:i,s:i,s:d,s:i,s:d,s:d,s:d,s:i}",
    "alt_diff", (long)stats.alt_diff,
    "duration", (long)stats.duration,
    "fraction", stats.fraction,
    "distance", (long)stats.distance,
    "speed", stats.GetSpeed(),
    "vario", stats.GetVario(),
    "glide_rate", stats.GetGlideRate(),
    "count", (long)stats.merges);
}

PyObject* Python::WritePerformanceStats(const PhaseTotals &totals) {
  return Py_BuildValue("{s:N,s:N,s:N,s:N,s:N}",
    "circling_total", WriteCirclingStats(totals.total_circstats),
    "circling_left", WriteCirclingStats(totals.left_circstats),
    "circling_right", WriteCirclingStats(totals.right_circstats),
    "circling_mixed", WriteCirclingStats(totals.mixed_circstats),
    "cruise_total", WriteCruiseStats(totals.total_cruisestats));
}

PyObject* Python::WriteWindItem(const WindListItem &wind_item) {
  return Py_BuildValue("{s:N,s:i,s:d,s:d}",
    "datetime", BrokenDateTimeToPy(wind_item.datetime),
    "altitude", (long)wind_item.altitude,
    "speed", wind_item.wind.norm,
    "direction", wind_item.wind.bearing.Degrees());
}

PyObject* Python::IGCFixEnhancedToPyTuple(const IGCFixEnhanced &fix) {
  PyObject *py_enl,
           *py_trt,
           *py_gsp,
           *py_tas,
           *py_ias,
           *py_siu,
           *py_elevation;

  if (fix.enl > -1) {
    py_enl = PyInt_FromLong(fix.enl);
  } else {
    py_enl = Py_None;
    Py_INCREF(Py_None);
  }

  if (fix.trt > -1) {
    py_trt = PyInt_FromLong(fix.trt);
  } else {
    py_trt = Py_None;
    Py_INCREF(Py_None);
  }

  if (fix.gsp > -1) {
    py_gsp = PyInt_FromLong(fix.gsp);
  } else {
    py_gsp = Py_None;
    Py_INCREF(Py_None);
  }

  if (fix.tas > -1) {
    py_tas = PyInt_FromLong(fix.tas);
  } else {
    py_tas = Py_None;
    Py_INCREF(Py_None);
  }

  if (fix.ias > -1) {
    py_ias = PyInt_FromLong(fix.ias);
  } else {
    py_ias = Py_None;
    Py_INCREF(Py_None);
  }

  if (fix.siu > -1) {
    py_siu = PyInt_FromLong(fix.siu);
  } else {
    py_siu = Py_None;
    Py_INCREF(Py_None);
  }

  if (fix.elevation > -999) {
    py_elevation = PyInt_FromLong(fix.elevation);
  } else {
    py_elevation = Py_None;
    Py_INCREF(Py_None);
  }

  return Py_BuildValue("(NiNiiNNNNNNNi)",
    BrokenDateTimeToPy(BrokenDateTime(fix.date, fix.time)),
    fix.clock,
    WriteLonLat(fix.location),
    fix.gps_altitude,
    fix.pressure_altitude,
    py_enl,
    py_trt,
    py_gsp,
    py_tas,
    py_ias,
    py_siu,
    py_elevation,
    fix.level);
}

bool Python::PyTupleToIGCFixEnhanced(PyObject *py_fix, IGCFixEnhanced &fix) {
  PyObject *py_datetime = nullptr,
           *py_location = nullptr,
           *py_gps_alt = nullptr,
           *py_pressure_alt = nullptr,
           *py_enl = nullptr,
           *py_trt = nullptr,
           *py_gsp = nullptr,
           *py_tas = nullptr,
           *py_ias = nullptr,
           *py_siu = nullptr,
           *py_elevation = nullptr,
           *py_level = nullptr;

  fix.Clear();

  if (!PyArg_ParseTuple(py_fix, "OiOO|OOOOOOOOO",
         &py_datetime, &fix.clock, &py_location,
         &py_gps_alt, &py_pressure_alt,
         &py_enl, &py_trt, &py_gsp, &py_tas, &py_ias,
         &py_siu, &py_elevation, &py_level)) {
    return false;
  }

  fix.date = PyToBrokenDateTime(py_datetime);
  fix.time = PyToBrokenDateTime(py_datetime);

  fix.location = ReadLonLat(py_location);

  if (!fix.location.IsValid())
    return false;

  if (!PyNumber_Check(py_gps_alt) && !PyNumber_Check(py_pressure_alt)) {
    PyErr_SetString(PyExc_ValueError, "Need at least gps or pressure altitude");
    return false;
  }

  if (PyNumber_Check(py_gps_alt)) {
    fix.gps_altitude = PyInt_AsLong(py_gps_alt);
    fix.gps_valid = true;
  } else {
    fix.gps_altitude = 0;
    fix.gps_valid = false;
  }

  if (PyNumber_Check(py_pressure_alt))
    fix.pressure_altitude = PyInt_AsLong(py_pressure_alt);
  else
    /* fall back to GPS altitude - this is the same behaviour as in IGCFix::Apply() */
    fix.pressure_altitude = fix.gps_altitude;


  if (PyNumber_Check(py_enl))
    fix.enl = PyInt_AsLong(py_enl);

  if (PyNumber_Check(py_trt))
    fix.trt = PyInt_AsLong(py_trt);

  if (PyNumber_Check(py_gsp))
    fix.gsp = PyInt_AsLong(py_gsp);

  if (PyNumber_Check(py_tas))
    fix.tas = PyInt_AsLong(py_tas);

  if (PyNumber_Check(py_ias))
    fix.ias = PyInt_AsLong(py_ias);

  if (PyNumber_Check(py_siu))
    fix.siu = PyInt_AsLong(py_siu);

  if (PyNumber_Check(py_elevation))
    fix.elevation = PyInt_AsLong(py_elevation);

  if (PyNumber_Check(py_level))
    fix.level = PyInt_AsLong(py_level);

  return true;
}

bool Python::PyStringToString(PyObject *py_string, tstring &string) {
#if PY_MAJOR_VERSION >= 3
  if (PyUnicode_Check(py_string)) {
    string.assign(PyUnicode_AsUTF8(py_string));
    return true;
  }
#else
  if (PyUnicode_Check(py_string)) {
    PyObject *py_string_utf8 = PyUnicode_AsUTF8String(py_string);
    string.assign(PyString_AsString(py_string_utf8));
    Py_DECREF(py_string_utf8);
    return true;
  } else if (PyString_Check(py_string)) {
    string.assign(PyString_AsString(py_string));
    return true;
  }
#endif

  return false;
}
