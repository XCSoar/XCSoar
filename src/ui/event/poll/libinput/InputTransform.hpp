// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The XCSoar Project

#pragma once

#include "DisplayOrientation.hpp"
#include "ui/dim/Point.hpp"
#include "ui/dim/Size.hpp"

#include <cstdint>

namespace UI {

enum class InputTransformMode : uint8_t {
  XCSOAR_ROTATED,
  SYSTEM_ROTATED,
  COUNT,
};

[[gnu::pure]]
inline PixelPoint
TransformInputPointForMode(PixelPoint p, PixelSize screen_size,
                           DisplayOrientation orientation,
                           InputTransformMode mode) noexcept
{
  if (mode == InputTransformMode::SYSTEM_ROTATED)
    return p;

  switch (TranslateDefaultDisplayOrientation(orientation)) {
  case DisplayOrientation::DEFAULT:
  case DisplayOrientation::LANDSCAPE:
    return p;

  case DisplayOrientation::PORTRAIT:
    return PixelPoint(screen_size.height - p.y, p.x);

  case DisplayOrientation::REVERSE_LANDSCAPE:
    return PixelPoint(screen_size.width - p.x,
                      screen_size.height - p.y);

  case DisplayOrientation::REVERSE_PORTRAIT:
    return PixelPoint(p.y, screen_size.width - p.x);
  }

  return p;
}

[[gnu::pure]]
inline PixelSize
GetLogicalInputSizeForMode(PixelSize screen_size,
                           DisplayOrientation orientation,
                           InputTransformMode mode) noexcept
{
  if (mode == InputTransformMode::XCSOAR_ROTATED &&
      AreAxesSwapped(orientation))
    return PixelSize(screen_size.height, screen_size.width);

  return screen_size;
}

} // namespace UI
