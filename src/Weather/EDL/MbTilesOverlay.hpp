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

namespace EDL {

class MbTilesOverlay final : public MapOverlay {
  MbTilesDatabase database;
  std::string label;
  unsigned cached_zoom = 0;
  std::map<TileKey, std::optional<MapOverlayBitmap>> cache;

  [[gnu::pure]]
  unsigned SelectZoom(const WindowProjection &projection) const noexcept;

  MapOverlayBitmap
  LoadTile(TileKey key);

public:
  MbTilesOverlay(Path path, std::string _label);

  const char *GetLabel() const noexcept override {
    return label.c_str();
  }

  bool IsInside(GeoPoint p) const noexcept override;
  void Draw(Canvas &canvas, const WindowProjection &projection) noexcept override;
};

} // namespace EDL
