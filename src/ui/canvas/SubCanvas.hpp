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
#endif

public:
  SubCanvas(Canvas &canvas, PixelPoint _offset, PixelSize _size) noexcept;

#ifdef ENABLE_OPENGL
  ~SubCanvas() noexcept;
#endif
};
