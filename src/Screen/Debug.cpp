// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The XCSoar Project

#include "Screen/Debug.hpp"

#ifndef NDEBUG

#include <cassert>

static bool screen_initialized = false;

void
ScreenInitialized()
{
  assert(!screen_initialized);
  screen_initialized = true;
}

void
ScreenDeinitialized()
{
  assert(screen_initialized);
  screen_initialized = false;
}

bool
IsScreenInitialized()
{
  return screen_initialized;
}

#endif
