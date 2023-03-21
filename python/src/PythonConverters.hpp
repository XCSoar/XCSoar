// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The XCSoar Project

#pragma once

#include <Python.h>

#include "util/tstring.hpp"
#include "time/Stamp.hpp"

struct BrokenDateTime;
struct GeoPoint;
struct ContestResult;
class ContestTraceVector;
struct ContestTracePoint;
struct Phase;
struct PhaseTotals;
struct WindListItem;
struct IGCFixEnhanced;

namespace Python {

  /**
   * Convert a XCSoar BrokenDateTime to a Python DateTime object
   */
  PyObject* BrokenDateTimeToPy(const BrokenDateTime &datetime);

  /**
   * Convert a Python DateTime object to XCSoar BrokenDateTime
   */
  BrokenDateTime PyToBrokenDateTime(PyObject *py_datetime);

  /**
   * Convert a GeoPoint to a python dict {longitude, latitude}
   */
  PyObject* WriteLonLat(const GeoPoint &location);
  GeoPoint ReadLonLat(PyObject *py_location);

  /**
   * Convert a event (datetime + location) to a python dict
   */
  PyObject* WriteEvent(const BrokenDateTime &datetime,
                       const GeoPoint &location);

  /**
   * Convert two points to a python dict with leg statistics
   */
  PyObject* WritePoint(const ContestTracePoint &point,
                       const ContestTracePoint *previous);

  PyObject* WriteContest(const ContestResult &result,
                         const ContestTraceVector &trace);

  PyObject* WritePhase(const Phase &phase);
  PyObject* WriteCirclingStats(const Phase &stats);
  PyObject* WriteCruiseStats(const Phase &stats);
  PyObject* WritePerformanceStats(const PhaseTotals &totals);

  PyObject* WriteWindItem(const WindListItem &wind_item);

  /**
   * Convert a IGCFixEnhanced to a tuple
   */
  PyObject* IGCFixEnhancedToPyTuple(const IGCFixEnhanced &fix);
  bool PyTupleToIGCFixEnhanced(PyObject *py_fix, IGCFixEnhanced &fix);

  /**
   * Convert a python string/unicode object to a tstring (aka std::[w]string)
   */
  bool PyStringToString(PyObject *py_string, tstring &string);

  TimeStamp PyLongToTimeStamp(PyObject* clock);
};
