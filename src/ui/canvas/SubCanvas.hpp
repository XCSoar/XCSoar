// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The XCSoar Project

#pragma once

#include "ui/canvas/Canvas.hpp"

#ifdef ENABLE_OPENGL
#include "ui/dim/Point.hpp"
#endif

/**
 * A #Canvas implementation which maps into a part of an existing
 * #Canvas.
 */
class SubCanvas : public Canvas {
#ifdef ENABLE_OPENGL
  PixelPoint relative;
#elif defined(USE_WINUSER)
  POINT old_viewport;
#endif

public:
  SubCanvas(Canvas &canvas, PixelPoint _offset, PixelSize _size) noexcept;

#if defined(ENABLE_OPENGL) || defined(USE_WINUSER)
  ~SubCanvas() noexcept;
#endif
};
