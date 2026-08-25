// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The XCSoar Project

#include "ui/canvas/BufferCanvas.hpp"

#include <algorithm>

#ifdef USE_MEMORY_CANVAS
/* Memory BufferCanvas is a VirtualCanvas subclass; Grow lives on the
   base (historically BufferCanvas was a typedef of VirtualCanvas). */
void
VirtualCanvas::Grow(PixelSize new_size) noexcept
#else
void
BufferCanvas::Grow(PixelSize new_size) noexcept
#endif
{
  const unsigned old_width = GetWidth();
  const unsigned old_height = GetHeight();

  Resize({std::max(old_width, new_size.width),
          std::max(old_height, new_size.height)});
}
