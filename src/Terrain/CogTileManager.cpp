// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The XCSoar Project

#include "CogTileManager.hpp"
#include "CogLoader.hpp"
#include "Operation/Operation.hpp"
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
CogTileManager::UpdateTiles(const GeoPoint &center, double radius,
                            [[maybe_unused]] RasterMap &map,
                            OperationEnvironment &operation) noexcept
{
  if (!center.IsValid() || curl == nullptr)
    return false;

  if (last_center.IsValid() && last_radius >= radius &&
      last_center.DistanceS(center) < 10000)
    return false;

  last_center = center;
  last_radius = radius;

  constexpr double DEGREES_PER_METER = 1.0 / 111000.0;
  const double radius_deg = radius * DEGREES_PER_METER * 1.5;

  const int min_lat = static_cast<int>(std::floor(center.latitude.Degrees() - radius_deg));
  const int max_lat = static_cast<int>(std::floor(center.latitude.Degrees() + radius_deg));
  const int min_lon = static_cast<int>(std::floor(center.longitude.Degrees() - radius_deg));
  const int max_lon = static_cast<int>(std::floor(center.longitude.Degrees() + radius_deg));

  bool loaded_new = false;

  for (int lat = min_lat; lat <= max_lat; ++lat) {
    for (int lon = min_lon; lon <= max_lon; ++lon) {
      CogTileId tile{lat, lon};

      if (IsTileLoaded(tile))
        continue;

      std::string url = GetTileUrl(tile);
      LogFormat("COG: Loading tile %s from %s", tile.ToFilename().c_str(), url.c_str());

      operation.SetText(tile.ToFilename().c_str());

      try {
        auto loader = std::make_unique<CogLoader>();
        if (loader->OpenHttp(*curl, url.c_str())) {
          const std::lock_guard lock{tiles_mutex};
          loaded_tiles[tile] = std::move(loader);
          loaded_new = true;
          LogFormat("COG: Successfully loaded tile %s", tile.ToFilename().c_str());
        } else {
          LogFormat("COG: Tile %s not available", tile.ToFilename().c_str());
        }
      } catch (...) {
        LogError(std::current_exception(), "COG: Failed to load tile");
      }
    }
  }

  return loaded_new;
}

TerrainHeight
CogTileManager::GetHeight(const GeoPoint &location) const noexcept
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
  return loader.GetHeight(location);
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
