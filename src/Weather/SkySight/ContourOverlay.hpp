// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The XCSoar Project

#pragma once

#include "ContourRaster.hpp"
#include "FieldImage.hpp"
#include "Layers.hpp"
#include "MapWindow/Overlay.hpp"
#include "MapWindow/OverlayBitmap.hpp"

#include <map>
#include <optional>
#include <string>

class Path;

#ifdef USE_GEOTIFF

/**
 * Draws a SkySight forecast field as filled contour bands.
 *
 * Forecast regions are continent-sized, so a pre-rendered image can only
 * hold one pixel per grid sample; magnifying that on the map blends
 * neighbouring legend colours into shades the legend does not contain.
 * This overlay keeps the scalar field instead and contours just the patch
 * on screen, which stays small at every zoom, so band boundaries land on
 * the iso-line of the field rather than on a grid cell edge.
 */
class SkySightContourOverlay final : public MapOverlay {
  SkySight::GeoScalarField source;
  SkySight::ContourPalette palette;
  GeoBounds bounds;
  std::string label;
  float alpha = 1;

  /** The patch most recently contoured, reused until the view leaves it. */
  std::optional<MapOverlayBitmap> cached;
  SkySight::ContourWindow cached_window;

  [[gnu::pure]]
  SkySight::ContourWindow
  SelectWindow(const WindowProjection &projection) const noexcept;

  /** @return false when the patch could not be rendered */
  bool Render(SkySight::ContourWindow window) noexcept;

public:
  /**
   * Load a field written by #WriteFieldImage.  Throws when the file is
   * not one, so a stale overlay never reaches the map.
   */
  SkySightContourOverlay(Path path,
                         const std::map<float, SkySight::LegendColor> &legend);

  void SetAlpha(float _alpha) noexcept {
    alpha = _alpha;
  }

  template<typename T>
  void SetLabel(T &&_label) noexcept {
    label = std::forward<T>(_label);
    if (cached)
      cached->SetLabel(label.c_str());
  }

  /* virtual methods from class MapOverlay */
  const char *GetLabel() const noexcept override {
    return label.c_str();
  }

  bool IsInside(GeoPoint p) const noexcept override;
  void Draw(Canvas &canvas,
            const WindowProjection &projection) noexcept override;
};

#endif
