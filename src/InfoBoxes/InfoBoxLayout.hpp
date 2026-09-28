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
   * of their own; while the InfoBox area is kept clear of it, they do.
   *
   * @see DisplaySettings::infobox_area_stretch
   */
  unsigned outer_border = 0;

  constexpr bool HasVario() const noexcept {
    return vario.right > vario.left && vario.bottom > vario.top;
  }

  void ClearVario() noexcept {
    vario.left = vario.top = vario.right = vario.bottom = 0;
  }
};

/**
 * Lay the InfoBoxes out in @p rc.
 *
 * @param orientation_size the screen the geometry was chosen for.
 * An empty size uses @p rc.  Pass the full screen when @p rc is a
 * smaller page, so a short page does not switch between rows and
 * columns.
 */
[[gnu::pure]]
Layout
Calculate(PixelRect rc, InfoBoxSettings::Geometry geometry,
          unsigned scale_title_font = 100,
          PixelSize orientation_size = {}) noexcept;

[[gnu::const]]
int
GetBorder(InfoBoxSettings::Geometry geometry, bool landscape,
          unsigned i) noexcept;

} // namespace InfoBoxLayout
