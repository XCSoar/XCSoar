// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The XCSoar Project

#pragma once

#include "Overlay.hpp"
#include "ui/canvas/Bitmap.hpp"
#include "Geo/Quadrilateral.hpp"
#include "Geo/GeoBounds.hpp"
#include "Geo/ReferencedGrid.hpp"

#include <cstdint>
#include <string>

class Canvas;
class WindowProjection;
enum class MapOverlayBlendMode : uint8_t;

/**
 * A georeferenced bitmap that can be rendered in the #MapWindow.
 *
 * @see MapWindow:SetOverlay()
 */
class MapOverlayBitmap final : public MapOverlay {
  Bitmap bitmap;

  /**
   * The geo reference grid for #bitmap. A 1x1 grid (the four corners)
   * for bitmaps loaded with a single quadrilateral, or a finer mesh for
   * GeoTIFF with curved projection.
   */
  GeoReferencedGrid grid;

  /**
   * The four outer corners of #grid, for the single-quadrilateral draw
   * path and for IsInside().
   */
  GeoQuadrilateral bounds;

  /**
   * The smallest rectangle that contains #bounds, for fast visibility
   * checking in Draw().
   */
  GeoBounds simple_bounds;

  bool use_bitmap_alpha = true;

  float alpha = 1;

  MapOverlayBlendMode blend_mode{};

  std::string label;

public:
  /**
   * Load a GeoTIFF file.
   *
   * Throws on error.
   */
#ifdef USE_GEOTIFF
  MapOverlayBitmap(Path path);
#else
  [[noreturn]] MapOverlayBitmap(Path path);
#endif

  /**
   * Move an existing #Bitmap with a geo reference.
   */
  MapOverlayBitmap(Bitmap &&_bitmap, GeoQuadrilateral _bounds,
                   std::string::const_pointer _label) noexcept
    :bitmap(std::move(_bitmap)), grid(_bounds), bounds(_bounds),
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

  void SetBlendMode(MapOverlayBlendMode _blend_mode) noexcept {
    blend_mode = _blend_mode;
  }

  /* virtual methods from class MapOverlay */
  const char *GetLabel() const noexcept override {
    return label.c_str();
  }

  bool IsInside(GeoPoint p) const noexcept override;
  void Draw(Canvas &canvas,
            const WindowProjection &projection) noexcept override;
};
