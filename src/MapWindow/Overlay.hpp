// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The XCSoar Project

#pragma once

#include "util/Compiler.h"

#include <tchar.h>

class Canvas;
class WindowProjection;
struct GeoPoint;

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
   * Draw the overlay to the given #Canvas.
   */
  virtual void Draw(Canvas &canvas,
                    const WindowProjection &projection) noexcept = 0;
};
