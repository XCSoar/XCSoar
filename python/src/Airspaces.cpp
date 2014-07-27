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

#include "Airspaces.hpp"

#include "Engine/Airspace/Airspaces.hpp"

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

PyMethodDef xcsoar_Airspaces_methods[] = {
  {NULL, NULL, 0, NULL}
};

PyMemberDef xcsoar_Airspaces_members[] = {
  {NULL}  /* Sentinel */
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
