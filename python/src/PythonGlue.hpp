// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The XCSoar Project

#pragma once

#include <Python.h>

#if PY_MAJOR_VERSION >= 3
  PyMODINIT_FUNC PyInit_xcsoar();
#else
  PyMODINIT_FUNC initxcsoar();
#endif
