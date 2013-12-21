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

