// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The XCSoar Project

#include "ui/canvas/Pen.hpp"
#include "Screen/Debug.hpp"

#include <cassert>

void
Pen::Create(Style _style, unsigned _width, const Color c)
{
  assert(IsScreenInitialized());

  width = _width;
  color = c;

#if defined(USE_MEMORY_CANVAS) || defined(ENABLE_OPENGL)
  style = _style;
#endif
}

void
Pen::Create(unsigned width, const Color c)
{
  Create(SOLID, width, c);
}
