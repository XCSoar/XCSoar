// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The XCSoar Project

#pragma once

#include "DisplayOrientation.hpp"
#include "ui/dim/Point.hpp"
#include "ui/dim/Size.hpp"

#include <algorithm>

#ifdef KOBO
#include "Kobo/Model.hpp"
#endif

namespace UI {

/**
 * This class knows how to rotate the coordinates of a pointer device
 * (touch screen) to match the coordinates of a rotated screen.
 */
class RotatePointer {
  /**
   * Swap x and y?
   */
  bool swap = false;

  /**
   * Invert x or y?
   */
  bool invert_x = false, invert_y = false;

  /**
   * Screen dimensions in pixels.
   */
  PixelSize screen_size{0, 0};

public:
  constexpr PixelSize GetScreenSize() const noexcept {
    return screen_size;
  }

  void SetScreenSize(PixelSize _screen_size) noexcept {
    screen_size = _screen_size;
  }

  void SetSwap(bool _swap) {
    swap = _swap;
  }

  void SetInvert(bool _invert_x, bool _invert_y) {
    invert_x = _invert_x;
    invert_y = _invert_y;
  }

  void SetDisplayOrientation(DisplayOrientation orientation) {
    SetSwap(AreAxesSwapped(orientation));
#ifdef KOBO
    KoboModel kobo_model = DetectKoboModel();
#endif

    switch (TranslateDefaultDisplayOrientation(orientation)) {
    case DisplayOrientation::DEFAULT:
    case DisplayOrientation::PORTRAIT:
#ifdef KOBO
      if (kobo_model == KoboModel::LIBRA2)
        SetInvert(false, false);
      else if (kobo_model == KoboModel::LIBRA_H2O)
        SetInvert(true, false);
      else
#endif
        SetInvert(true, false);
      break;

    case DisplayOrientation::LANDSCAPE:
#ifdef KOBO
      if (kobo_model == KoboModel::LIBRA2)
        SetInvert(false, true);
      else if (kobo_model == KoboModel::LIBRA_H2O)
        SetInvert(false, false);
      else
#endif
        SetInvert(false, false);
      break;

    case DisplayOrientation::REVERSE_PORTRAIT:
#ifdef KOBO
      if (kobo_model == KoboModel::LIBRA2)
        SetInvert(true, true);
      else if (kobo_model == KoboModel::LIBRA_H2O)
        SetInvert(false, true);
      else
#endif
        SetInvert(false, true);
      break;

    case DisplayOrientation::REVERSE_LANDSCAPE:
#ifdef KOBO
      if (kobo_model == KoboModel::LIBRA2)
        SetInvert(true, false);
      else if (kobo_model == KoboModel::LIBRA_H2O)
        SetInvert(true, true);
      else
#endif
        SetInvert(true, true);
      break;
    }
  }

  [[gnu::pure]]
  PixelPoint DoRelative(PixelPoint p) const {
    if (swap)
      std::swap(p.x, p.y);

    return p;
  }

  [[gnu::pure]]
  PixelPoint DoAbsolute(PixelPoint p) const {
    p = DoRelative(p);

    if (invert_x)
      p.x = screen_size.width - p.x;

    if (invert_y)
      p.y = screen_size.height - p.y;

    return p;
  }
};

} // namespace UI
