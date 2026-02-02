// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The XCSoar Project

#pragma once

#ifdef SOFTWARE_ROTATE_DISPLAY

#include "ui/dim/Point.hpp"
#include "ui/dim/Size.hpp"
#include "DisplayOrientation.hpp"
#include "ui/canvas/opengl/Globals.hpp"

namespace UI {

/**
 * Transform physical coordinates to logical coordinates for
 * SOFTWARE_ROTATE_DISPLAY. This handles the case where only the rendering
 * is rotated, not the physical screen.
 */
[[gnu::pure]]
inline PixelPoint
TransformCoordinates(PixelPoint p, PixelSize physical_size) noexcept
{
  const auto orientation = OpenGL::display_orientation;

  switch (TranslateDefaultDisplayOrientation(orientation)) {
  case DisplayOrientation::DEFAULT:
  case DisplayOrientation::LANDSCAPE:
    return p;

  case DisplayOrientation::PORTRAIT:
    return PixelPoint(physical_size.height - p.y, p.x);

  case DisplayOrientation::REVERSE_LANDSCAPE:
    return PixelPoint(physical_size.width - p.x,
                      physical_size.height - p.y);

  case DisplayOrientation::REVERSE_PORTRAIT:
    return PixelPoint(p.y, physical_size.width - p.x);
  }

  return p;
}

} // namespace UI

#endif

