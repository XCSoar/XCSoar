// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The XCSoar Project

#include "ui/canvas/Pen.hpp"
#include "Screen/Debug.hpp"

#include <cassert>

void
Pen::Create(Style Style, unsigned width, const Color c)
{
  assert(IsScreenInitialized());

  Destroy();
  pen = ::CreatePen(Style, width, c);
}

void
Pen::Create(unsigned width, const Color c)
{
  Create(SOLID, width, c);
}

void
Pen::Destroy() noexcept
{
  assert(!IsDefined() || IsScreenInitialized());

  if (pen != nullptr) {
#ifndef NDEBUG
    bool success =
#endif
      ::DeleteObject(pen);
    assert(success);

    pen = nullptr;
  }
}
