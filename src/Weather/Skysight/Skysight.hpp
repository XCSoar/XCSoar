// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The XCSoar Project

#pragma once

#include "Layers.hpp"
#include "ui/canvas/custom/GeoBitmap.hpp"
#include "system/Path.hpp"

#include <array>
#include <memory>
#include <string>
#include <string_view>
#include <vector>

struct SkysightRegionEntry;

class CurlGlobal;
class Path;
class SkysightAPI;

class Skysight final {
  std::unique_ptr<SkysightAPI> api;
  SkySight::Layer *active_layer = nullptr;
  SkySight::Layer *displayed_layer = nullptr;
  unsigned displayed_zoom = 0;
  std::array<std::string, 9> tile_filenames;

public:
  explicit Skysight(CurlGlobal &curl);
  ~Skysight();

  void Init();

  [[nodiscard]] static AllocatedPath GetLocalPath() noexcept;

  std::size_t NumLayers() const noexcept;
  const SkySight::Layer *GetLayer(std::size_t index) const noexcept;
  const std::vector<SkysightRegionEntry> &GetRegions() const noexcept;
  std::string_view GetRegion() const noexcept;

  bool HasCredentials() const noexcept;

  std::string_view GetActiveLayerId() const noexcept;

  bool SetLayerActive(std::string_view id);
  void DeactivateLayer();
  void Render();

  void OnDataUpdated() noexcept;

private:
  void CleanupFiles() noexcept;
  void ResetTiles() noexcept;
  bool UpdateActiveLayer(unsigned index, Path path,
                         const GeoBitmap::TileData &tile);
  bool DisplayTileLayer();
};