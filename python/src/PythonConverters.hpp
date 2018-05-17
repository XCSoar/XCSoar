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

#ifndef PYTHON_PYTHONCONVERTERS_HPP
#define PYTHON_PYTHONCONVERTERS_HPP

#include <Python.h>

#include "Util/tstring.hpp"

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
};

#endif /* PYTHON_PYTHONCONVERTERS_HPP */
