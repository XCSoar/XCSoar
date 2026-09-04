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

  /**
   * The border flags (see #BorderKind_t) of each InfoBox.
   */
  int borders[InfoBoxSettings::Panel::MAX_CONTENTS];

  /**
   * Is this InfoBox displayed at all?  ApplyContents() clears this
   * for InfoBoxes which have released their space.
   */
  bool visible[InfoBoxSettings::Panel::MAX_CONTENTS];

  PixelRect vario;

  PixelRect remaining;

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

/**
 * Redistribute the InfoBox positions of the given layout according to
 * the contents of the given panel.  An InfoBox which claims no space
 * of its own (#InfoBoxFactory::e_ReleaseSpace) becomes invisible, and the
 * remaining InfoBoxes of the same row (or column) grow into the gap.
 */
void
ApplyContents(Layout &layout,
              const InfoBoxSettings::Panel &panel) noexcept;

} // namespace InfoBoxLayout
