// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The XCSoar Project

#pragma once

#include "Overlay.hpp"
#include "ui/canvas/Bitmap.hpp"
#include "Geo/Quadrilateral.hpp"
#include "Geo/GeoBounds.hpp"
#include "util/tstring.hpp"

class Canvas;
class WindowProjection;

/**
 * A georeferenced bitmap that can be rendered in the #MapWindow.
 *
 * @see MapWindow:SetOverlay()
 */
class MapOverlayBitmap final : public MapOverlay {
  Bitmap bitmap;

  /**
   * The geo reference for #bitmap.
   */
  GeoQuadrilateral bounds;

  /**
   * The smallest rectangle that contains #bounds, for fast visibility
   * checking in Draw().
   */
  GeoBounds simple_bounds;

  bool use_bitmap_alpha = true;

  float alpha = 1;

  tstring label;

public:
  /**
   * Load a GeoTIFF file.
   *
   * Throws on error.
   */
  MapOverlayBitmap(Path path);

  /**
   * Move an existing #Bitmap with a geo reference.
   */
  MapOverlayBitmap(Bitmap &&_bitmap, GeoQuadrilateral _bounds,
                   tstring::const_pointer _label) noexcept
    :bitmap(std::move(_bitmap)), bounds(_bounds),
     simple_bounds(bounds.GetBounds()),
     label(_label) {}

  template<typename T>
  void SetLabel(T &&_label) noexcept {
    label = std::forward<T>(_label);
  }

  /**
   * By default, this class uses the bitmap's alpha channel.  This
   * method disables the alpha channel.
   */
  void IgnoreBitmapAlpha() noexcept {
    use_bitmap_alpha = false;
  }

  /**
   * Apply a constant alpha value.
   */
  void SetAlpha(float _alpha) noexcept {
    alpha = _alpha;
  }

  /* virtual methods from class MapOverlay */
  const char *GetLabel() const noexcept override {
    return label.c_str();
  }

  bool IsInside(GeoPoint p) const noexcept override;
  void Draw(Canvas &canvas,
            const WindowProjection &projection) noexcept override;
};
