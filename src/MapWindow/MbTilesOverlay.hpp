// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The XCSoar Project

#pragma once

#include "MapWindow/OverlayBitmap.hpp"
#include "MapWindow/Overlay.hpp"
#include "MbTilesDatabase.hpp"
#include "system/Path.hpp"

#include <cstdint>
#include <compare>
#include <map>
#include <optional>
#include <string>

class MbTilesOverlay : public MapOverlay {
  MbTilesDatabase database;
  std::string label;
  unsigned cached_zoom = 0;
  std::map<TileKey, std::optional<MapOverlayBitmap>> cache;

  [[gnu::pure]]
  unsigned SelectZoom(const WindowProjection &projection) const noexcept;

  MapOverlayBitmap LoadTile(TileKey key);

protected:
  const MbTilesDatabase &
  GetDatabase() const noexcept
  {
    return database;
  }

  bool
  SampleRgbaAtGeo(GeoPoint p, Rgba8 &pixel) const noexcept
  {
    return database.SampleRgbaAtGeo(p, pixel);
  }

public:
  MbTilesOverlay(Path path, std::string _label);

  const char *GetLabel() const noexcept override {
    return label.c_str();
  }

  bool IsInside(GeoPoint p) const noexcept override;
  void Draw(Canvas &canvas, const WindowProjection &projection) noexcept override;
};
