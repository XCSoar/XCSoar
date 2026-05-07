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
  bool forecast_image_dirty = true;
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
  std::size_t NumSelectedLayers() const noexcept;
  const SkySight::Layer *GetSelectedLayer(std::size_t index) const noexcept;
  const SkySight::Layer *GetSelectedLayer(std::string_view id) const noexcept;
  bool IsSelectedLayer(std::string_view id) const noexcept;
  bool SelectedLayersFull() const noexcept;
  bool AddSelectedLayer(std::string_view id);
  bool RemoveSelectedLayer(std::string_view id);
  bool HasForecastLayers() const noexcept;
  void RefreshCatalog() noexcept;

  bool HasCredentials() const noexcept;

  std::string_view GetActiveLayerId() const noexcept;
  std::string_view GetDisplayedLayerId() const noexcept;

  bool SetLayerActive(std::string_view id);
  void DeactivateLayer();
  void Render();

  void ReloadSelectedLayersFromProfile();
  void OnLayerCatalogChanged(std::string_view active_id,
                             std::string_view displayed_id) noexcept;
  void OnDataUpdated() noexcept;

private:
  bool AddSelectedLayer(std::string_view id, bool save_profile);
  void SaveSelectedLayers() const;
  void CleanupFiles() noexcept;
  void ResetTiles() noexcept;
  bool UpdateActiveLayer(unsigned index, Path path,
                         const GeoBitmap::TileData &tile);
  bool DisplayForecastLayer();
  bool DisplayTileLayer();
};