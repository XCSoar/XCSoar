// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The XCSoar Project

/* This file exists to avoid an object name collision in driver.a:
   multiple drivers had a translation unit named "Driver.cpp", which
   becomes "Driver.o" and breaks static linking. */

#include "Driver.cpp"

