// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The XCSoar Project

#pragma once

#include "InfoBoxSettings.hpp"
#include "ui/dim/Rect.hpp"

namespace InfoBoxLayout {

struct Layout {
  InfoBoxSettings::Geometry geometry;

  bool landscape;

  PixelSize control_size;

  unsigned count;
  PixelRect positions[InfoBoxSettings::Panel::MAX_CONTENTS];

  PixelRect vario;

  PixelRect remaining;

  /**
   * The area this layout was calculated for.
   */
  PixelRect rc;

  /**
   * Border flags for the InfoBoxes at the outer edge of #rc.  Those
   * edges usually coincide with the screen border and need no border
   * of their own; while the safe area is kept clear of it, they do.
   *
   * @see DisplaySettings::safe_area_stretch
   */
  unsigned outer_border = 0;

  constexpr bool HasVario() const noexcept {
    return vario.right > vario.left && vario.bottom > vario.top;
  }

  void ClearVario() noexcept {
    vario.left = vario.top = vario.right = vario.bottom = 0;
  }
};

[[gnu::pure]]
Layout
Calculate(PixelRect rc, InfoBoxSettings::Geometry geometry,
          unsigned scale_title_font=100) noexcept;

[[gnu::const]]
int
GetBorder(InfoBoxSettings::Geometry geometry, bool landscape,
          unsigned i) noexcept;

} // namespace InfoBoxLayout
