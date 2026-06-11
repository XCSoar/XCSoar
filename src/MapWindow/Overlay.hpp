// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The XCSoar Project

#pragma once

#include "util/Compiler.h"
#include "Geo/GeoPoint.hpp"

#include <cstddef>

class Canvas;
class WindowProjection;

/**
 * An overlay that can be rendered in the #MapWindow.
 *
 * @see MapWindow:SetOverlay()
 */
class MapOverlay {
public:
  virtual ~MapOverlay() noexcept = default;

  /**
   * Returns a human-readable name for this overlay.
   */
  [[gnu::pure]]
  virtual const char *GetLabel() const noexcept = 0;

  /**
   * Check whether the given location is inside the overlay.
   */
  [[gnu::pure]]
  virtual bool IsInside(GeoPoint p) const noexcept = 0;

  /**
   * Format human-readable detail about the overlay value at the given
   * location into @p buffer (NUL-terminated). Used by the map-item
   * list to show e.g. the forecast value under the user's tap.
   *
   * Default: no detail available. Overlays that can resolve a value at
   * a point (e.g. the XCTherm wind forecast) override this.
   *
   * @return true if any detail was written, false if not.
   */
  virtual bool FormatPointInfo([[maybe_unused]] GeoPoint p,
                               [[maybe_unused]] char *buffer,
                               [[maybe_unused]] std::size_t size) const noexcept {
    return false;
  }

  /**
   * Draw the overlay to the given #Canvas.
   */
  virtual void Draw(Canvas &canvas,
                    const WindowProjection &projection) noexcept = 0;
};
