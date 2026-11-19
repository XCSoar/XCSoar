// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The XCSoar Project

#include "CogTileManager.hpp"
#include "CogLoader.hpp"
#include "system/Path.hpp"
#include "system/FileUtil.hpp"
#include "LogFile.hpp"

#include <cstdio>
#include <cmath>
#include <cstring>

std::string
CogTileId::ToFilename() const noexcept
{
  char buf[16];
  snprintf(buf, sizeof(buf), "%c%02d%c%03d.tif",
           lat >= 0 ? 'N' : 'S', std::abs(lat),
           lon >= 0 ? 'E' : 'W', std::abs(lon));
  return buf;
}

const char *
CogTileManager::GetProviderUrl(Provider p) noexcept
{
  switch (p) {
  case Provider::OPENTOPOGRAPHY_SRTM_GL1:
    return "https://opentopography.s3.sdsc.edu/raster/SRTM_GL1/SRTM_GL1_srtm/";

  case Provider::OPENTOPOGRAPHY_SRTM_GL3:
    return "https://opentopography.s3.sdsc.edu/raster/SRTM_GL3/SRTM_GL3_srtm/";

  case Provider::CUSTOM:
    return nullptr;
  }

  return nullptr;
}

CogTileManager::Provider
CogTileManager::ParseProvider(const char *name) noexcept
{
  if (name == nullptr)
    return Provider::OPENTOPOGRAPHY_SRTM_GL1;

  if (strcmp(name, "srtm_gl1") == 0 || strcmp(name, "SRTM_GL1") == 0)
    return Provider::OPENTOPOGRAPHY_SRTM_GL1;

  if (strcmp(name, "srtm_gl3") == 0 || strcmp(name, "SRTM_GL3") == 0)
    return Provider::OPENTOPOGRAPHY_SRTM_GL3;

  return Provider::CUSTOM;
}

void
CogTileManager::SetProvider(Provider p, CurlGlobal &_curl) noexcept
{
  provider = p;
  curl = &_curl;

  const char *url = GetProviderUrl(p);
  if (url != nullptr)
    base_url = url;
}

void
CogTileManager::SetCustomUrl(const char *url, CurlGlobal &_curl) noexcept
{
  provider = Provider::CUSTOM;
  curl = &_curl;
  base_url = url;
}

std::string
CogTileManager::GetCachePath(const CogTileId &tile) const noexcept
{
  if (cache_path.empty())
    return {};

  const char *provider_dir = nullptr;
  switch (provider) {
  case Provider::OPENTOPOGRAPHY_SRTM_GL1:
    provider_dir = "srtm_gl1";
    break;
  case Provider::OPENTOPOGRAPHY_SRTM_GL3:
    provider_dir = "srtm_gl3";
    break;
  case Provider::CUSTOM:
    provider_dir = "custom";
    break;
  }

  char buf[256];
  snprintf(buf, sizeof(buf), "%s/%s/%s",
           cache_path.c_str(), provider_dir, tile.ToFilename().c_str());
  return buf;
}

std::string
CogTileManager::GetTileUrl(const CogTileId &tile) const noexcept
{
  if (provider == Provider::CUSTOM) {
    std::string url = base_url;

    char lat_str[8], lon_str[8];
    snprintf(lat_str, sizeof(lat_str), "%02d", std::abs(tile.lat));
    snprintf(lon_str, sizeof(lon_str), "%03d", std::abs(tile.lon));

    size_t pos;
    while ((pos = url.find("{N}")) != std::string::npos)
      url.replace(pos, 3, tile.lat >= 0 ? "N" : "S");
    while ((pos = url.find("{S}")) != std::string::npos)
      url.replace(pos, 3, tile.lat < 0 ? "S" : "N");
    while ((pos = url.find("{E}")) != std::string::npos)
      url.replace(pos, 3, tile.lon >= 0 ? "E" : "W");
    while ((pos = url.find("{W}")) != std::string::npos)
      url.replace(pos, 3, tile.lon < 0 ? "W" : "E");
    while ((pos = url.find("{LAT}")) != std::string::npos)
      url.replace(pos, 5, lat_str);
    while ((pos = url.find("{LON}")) != std::string::npos)
      url.replace(pos, 5, lon_str);

    return url;
  }

  return base_url + tile.ToFilename();
}

bool
CogTileManager::IsTileLoaded(const CogTileId &tile) const noexcept
{
  const std::lock_guard lock{tiles_mutex};
  return loaded_tiles.find(tile) != loaded_tiles.end();
}

size_t
CogTileManager::GetLoadedTileCount() const noexcept
{
  const std::lock_guard lock{tiles_mutex};
  return loaded_tiles.size();
}

void
CogTileManager::Clear() noexcept
{
  const std::lock_guard lock{tiles_mutex};
  loaded_tiles.clear();
}

bool
CogTileManager::UpdateTiles(const GeoPoint &center, double radius) noexcept
{
  if (!center.IsValid())
    return false;

  constexpr double DEGREES_PER_METER = 1.0 / 111000.0;
  const double radius_deg = radius * DEGREES_PER_METER * 1.5;

  const int min_lat = static_cast<int>(std::floor(center.latitude.Degrees() - radius_deg));
  const int max_lat = static_cast<int>(std::floor(center.latitude.Degrees() + radius_deg));
  const int min_lon = static_cast<int>(std::floor(center.longitude.Degrees() - radius_deg));
  const int max_lon = static_cast<int>(std::floor(center.longitude.Degrees() + radius_deg));

  const std::lock_guard lock{tiles_mutex};

  for (int lat = min_lat; lat <= max_lat; ++lat) {
    for (int lon = min_lon; lon <= max_lon; ++lon) {
      CogTileId tile{lat, lon};

      if (loaded_tiles.find(tile) != loaded_tiles.end())
        continue;

      if (failed_tiles.find(tile) != failed_tiles.end())
        continue;

      if (pending_tiles.find(tile) != pending_tiles.end())
        continue;

      pending_tiles.insert(tile);
    }
  }

  return !pending_tiles.empty();
}

bool
CogTileManager::ProcessOneRequest() noexcept
{
  CogTileId tile{0, 0};

  {
    const std::lock_guard lock{tiles_mutex};
    if (pending_tiles.empty())
      return false;

    tile = *pending_tiles.begin();
    pending_tiles.erase(pending_tiles.begin());
  }

  std::string path = GetCachePath(tile);
  if (!path.empty() && File::Exists(Path(path.c_str()))) {
    try {
      auto loader = std::make_unique<CogLoader>();
      if (loader->Open(Path(path.c_str()))) {
        const std::lock_guard lock{tiles_mutex};
        loaded_tiles[tile] = std::move(loader);
        LogFormat("COG: Loaded tile %s from cache", tile.ToFilename().c_str());
        return true;
      }
    } catch (...) {
    }
  }

  if (curl == nullptr) {
    const std::lock_guard lock{tiles_mutex};
    failed_tiles.insert(tile);
    return true;
  }

  std::string url = GetTileUrl(tile);
  LogFormat("COG: Loading tile %s from %s", tile.ToFilename().c_str(), url.c_str());

  try {
    auto loader = std::make_unique<CogLoader>();
    if (loader->OpenHttp(*curl, url.c_str())) {
      const std::lock_guard lock{tiles_mutex};
      loaded_tiles[tile] = std::move(loader);
      LogFormat("COG: Successfully loaded tile %s", tile.ToFilename().c_str());
      return true;
    } else {
      LogFormat("COG: Tile %s not available", tile.ToFilename().c_str());
      const std::lock_guard lock{tiles_mutex};
      failed_tiles.insert(tile);
    }
  } catch (...) {
    LogError(std::current_exception(), "COG: Failed to load tile");
    const std::lock_guard lock{tiles_mutex};
    failed_tiles.insert(tile);
  }

  return true;
}

TerrainHeight
CogTileManager::GetHeight(const GeoPoint &location,
                          unsigned quantisation) const noexcept
{
  if (!location.IsValid())
    return TerrainHeight::Invalid();

  CogTileId tile_id{
    static_cast<int>(std::floor(location.latitude.Degrees())),
    static_cast<int>(std::floor(location.longitude.Degrees()))
  };

  const std::lock_guard lock{tiles_mutex};
  auto it = loaded_tiles.find(tile_id);
  if (it == loaded_tiles.end())
    return TerrainHeight::Invalid();

  const auto &loader = *it->second;
  return loader.GetHeight(location, quantisation);
}

GeoBounds
CogTileManager::GetBounds() const noexcept
{
  const std::lock_guard lock{tiles_mutex};

  if (loaded_tiles.empty())
    return GeoBounds::Invalid();

  GeoBounds bounds = GeoBounds::Invalid();
  for (const auto &[tile_id, loader] : loaded_tiles) {
    const auto &tile_bounds = loader->GetBounds();
    if (tile_bounds.IsValid()) {
      bounds.Extend(tile_bounds.GetNorthWest());
      bounds.Extend(tile_bounds.GetSouthEast());
    }
  }

  return bounds;
}

unsigned
CogTileManager::PreCacheTiles(const GeoBounds &bounds,
                              unsigned quantisation) const noexcept
{
  if (!bounds.IsValid())
    return 0;

  const std::lock_guard lock{tiles_mutex};

  unsigned total_loaded = 0;
  for (const auto &[tile_id, loader] : loaded_tiles) {
    const auto &tile_bounds = loader->GetBounds();
    if (tile_bounds.IsValid() && tile_bounds.Overlaps(bounds)) {
      total_loaded += loader->PreCacheTiles(bounds, quantisation);
    }
  }

  return total_loaded;
}
