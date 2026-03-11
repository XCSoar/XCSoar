// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The XCSoar Project

#pragma once

#include "MapWindow/Overlay.hpp"
#include "MbTilesDatabase.hpp"
#include "system/Path.hpp"
#include "ui/canvas/Bitmap.hpp"

#include <compare>
#include <map>
#include <memory>
#include <string>

namespace EDL {

class MbTilesOverlay final : public MapOverlay {
  struct TileKey {
    unsigned zoom;
    unsigned column;
    unsigned row;

    constexpr auto operator<=>(const TileKey &) const noexcept = default;
  };

  MbTilesDatabase database;
  std::string label;
  uint8_t alpha = 140;
  unsigned cached_zoom = 0;
  std::map<TileKey, Bitmap> cache;

  [[gnu::pure]]
  unsigned SelectZoom(const WindowProjection &projection) const noexcept;

  Bitmap
  LoadTile(unsigned zoom, unsigned column, unsigned row);

public:
  MbTilesOverlay(Path path, std::string _label, uint8_t _alpha=140);

  const char *GetLabel() const noexcept override {
    return label.c_str();
  }

  bool IsInside(GeoPoint p) const noexcept override;
  void Draw(Canvas &canvas, const WindowProjection &projection) noexcept override;
};

} // namespace EDL
