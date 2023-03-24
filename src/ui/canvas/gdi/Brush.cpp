// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The XCSoar Project

#include "ui/canvas/Brush.hpp"
#include "ui/canvas/Bitmap.hpp"

#include <cassert>

void
Brush::Create(const Color c)
{
  assert(IsScreenInitialized());

  Destroy();
  brush = ::CreateSolidBrush(c);
}

#ifdef HAVE_HATCHED_BRUSH

void
Brush::Create(const Bitmap &bitmap)
{
  /* GDI works best when the bitmap is 8x8 - to avoid bad performance,
     disallow using any other bitmap size */
  assert(bitmap.GetSize().width == 8);
  assert(bitmap.GetSize().height == 8);

  Destroy();
  brush = ::CreatePatternBrush(bitmap.GetNative());
}

#endif

void
Brush::Destroy() noexcept
{
  assert(!IsDefined() || IsScreenInitialized());

  if (brush != nullptr) {
#ifndef NDEBUG
    bool success =
#endif
      ::DeleteObject(brush);
    assert(success);

    brush = nullptr;
  }
}
