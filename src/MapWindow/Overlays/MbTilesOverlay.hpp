// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The XCSoar Project

#pragma once

#include "MapWindow/Overlay.hpp"
#include "MapWindow/OverlayBitmap.hpp"
#include "MbTilesDatabase.hpp"

#include <map>
#include <optional>
#include <string>

namespace MbTiles {

class Overlay final : public MapOverlay {
  Database database;
  std::string label;
  unsigned cached_zoom = 0;
  std::map<TileKey, std::optional<MapOverlayBitmap>> cache;

  [[gnu::pure]]
  unsigned SelectZoom(const WindowProjection &projection) const noexcept;

  MapOverlayBitmap LoadTile(TileKey key);

public:
  Overlay(Path path, std::string _label);

  const char *GetLabel() const noexcept override {
    return label.c_str();
  }

  bool IsInside(GeoPoint p) const noexcept override;
  void Draw(Canvas &canvas, const WindowProjection &projection) noexcept override;
};

} // namespace MbTiles

