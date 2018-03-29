/* Copyright_License {

  XCSoar Glide Computer - http://www.xcsoar.org/
  Copyright (C) 2000-2015 The XCSoar Project
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
#include "Flight.hpp"
#include "Airspaces.hpp"
#include "Util.hpp"


PyMethodDef xcsoar_methods[] = {
  {"encode", (PyCFunction)xcsoar_encode, METH_VARARGS | METH_KEYWORDS, "Encode a list of numbers."},
  {nullptr, nullptr, 0, nullptr}
};

#if PY_MAJOR_VERSION >= 3
  #define MOD_INIT(name) \
          PyMODINIT_FUNC \
          PyInit_##name()
  #define MOD_DEF(ob, name, doc, methods) \
          static struct PyModuleDef moduledef = { \
            PyModuleDef_HEAD_INIT, name, doc, -1, methods, }; \
          ob = PyModule_Create(&moduledef);
  #define MOD_ERROR_VAL NULL
  #define MOD_SUCCESS_VAL(val) val

#else
  #define MOD_INIT(name) \
          PyMODINIT_FUNC \
          __attribute__ ((visibility("default"))) \
          init##name()
  #define MOD_DEF(ob, name, doc, methods) \
          ob = Py_InitModule3(name, methods, doc);
  #define MOD_ERROR_VAL
  #define MOD_SUCCESS_VAL(val)
#endif

MOD_INIT(xcsoar) {
  PyObject *m;

  MOD_DEF(m, "xcsoar", "XCSoar Tools", xcsoar_methods)

  if (m == nullptr)
    return MOD_ERROR_VAL;

  PyDateTime_IMPORT;

  if (!Flight_init(m))
    return MOD_ERROR_VAL;

  if (!Airspaces_init(m))
    return MOD_ERROR_VAL;

  return MOD_SUCCESS_VAL(m);
}
