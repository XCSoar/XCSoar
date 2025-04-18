// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The XCSoar Project

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
