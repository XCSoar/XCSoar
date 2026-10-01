// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The XCSoar Project

#pragma once

#include "ui/dim/Rect.hpp"

/**
 * Where in-flight map chrome sits inside the HUD.  Built once per
 * paint from the HUD rectangle and the overlay margins; draw code
 * reads slots instead of composing insets itself.
 *
 * Projection-space items (crosshair, aircraft, wind at the aircraft)
 * are not part of this layout.  The gesture label and the page
 * indicator keep their own optical offset from the HUD edge.
 */
struct MapHudLayout {
  /** HUD inset by the overlay-button padding. */
  PixelRect content;

  /** #content, right edge clear of the overlay-button column. */
  PixelRect top_right;

  /** #content, bottom edge clear of the portrait pan menu. */
  PixelRect bottom;

  /** How far the pan readout drops while the north arrow is drawn. */
  unsigned compass_slot = 0;

  /** How far above #bottom the GPS status sits (scale bar + title). */
  unsigned scale_title_clearance = 0;

  /**
   * The same gap the overlay buttons leave inside the HUD: one text
   * padding on every side.
   */
  [[gnu::pure]]
  static PixelRect InsetPadding(PixelRect rc) noexcept;

  [[gnu::pure]]
  static MapHudLayout Build(PixelRect hud,
                            unsigned top_right_margin,
                            unsigned bottom_margin,
                            unsigned compass_slot,
                            unsigned scale_title_clearance) noexcept;

  /** Top fifth of #content, along its left edge. */
  [[gnu::pure]]
  PixelRect GetThermalBandRect() const noexcept;

  /** Top of the pan elevation / coordinate column. */
  [[gnu::pure]]
  PixelPoint GetPanInfoOrigin(bool compass_visible) const noexcept;
};
