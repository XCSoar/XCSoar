// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The XCSoar Project

#include "SkysightAPI.hpp"
#include "SkySightRequest.hpp"
#include "Skysight.hpp"
#include "time/Convert.hxx"
#include "util/StaticString.hxx"

#include <chrono>
#include <ctime>

SkysightAPI::SkysightAPI(Skysight &_owner, CurlGlobal &curl, Path _cache_path)
  :owner(_owner),
   request(std::make_unique<SkySightRequest>(*this, curl)),
   cache_path(_cache_path)
{
  InitialiseLayers(layers);
}

SkysightAPI::~SkysightAPI() = default;

void
SkysightAPI::InitialiseLayers(std::vector<SkySight::Layer> &new_layers)
{
  new_layers.clear();

  new_layers.push_back({
    "satellite",
    "Satellite",
    "Live SkySight satellite tiles",
    true,
    true,
    1,
    8,
    1.0f,
  });

  new_layers.push_back({
    "rain",
    "Rain",
    "Live SkySight precipitation tiles",
    true,
    true,
    1,
    8,
    0.7f,
  });
}

void
SkysightAPI::Configure(std::string_view email, std::string_view password,
                       std::string_view new_region)
{
  region = FindSkysightRegionById(new_region.empty()
                                  ? std::string_view{GetDefaultSkysightRegion().id}
                                  : new_region).id;
  request->Configure(email, password);
}

bool
SkysightAPI::HasCredentials() const noexcept
{
  return request->HasCredentials();
}

const SkySight::Layer *
SkysightAPI::GetLayer(std::size_t index) const noexcept
{
  return index < layers.size()
    ? &layers[index]
    : nullptr;
}

SkySight::Layer *
SkysightAPI::GetLayer(std::string_view id) noexcept
{
  for (auto &i : layers)
    if (i == id)
      return &i;

  return nullptr;
}

std::string
SkysightAPI::FormatUrlTimestamp(time_t timestamp)
{
  const auto tm = GmTime(std::chrono::system_clock::from_time_t(timestamp));

  char buffer[32];
  std::strftime(buffer, sizeof(buffer), "%Y/%m/%d/%H%M", &tm);
  return buffer;
}

std::string
SkysightAPI::FormatFileTimestamp(time_t timestamp)
{
  const auto tm = GmTime(std::chrono::system_clock::from_time_t(timestamp));

  char buffer[32];
  std::strftime(buffer, sizeof(buffer), "%Y-%m-%d-%H%M", &tm);
  return buffer;
}

std::string
SkysightAPI::MakeTileUrl(const SkySight::Layer &layer,
                         time_t timestamp,
                         const GeoBitmap::TileData &tile)
{
  StaticString<256> url;
  url.Format("https://skysight.io/api/%s/%u/%u/%u/%s",
             layer.id.c_str(), tile.zoom, tile.x, tile.y,
             FormatUrlTimestamp(timestamp).c_str());
  return url.c_str();
}

AllocatedPath
SkysightAPI::GetTilePath(const SkySight::Layer &layer, time_t timestamp,
                         const GeoBitmap::TileData &tile) const
{
  StaticString<128> filename;
  filename.Format("%s-%u-%u-%u-%s.jpg",
                  layer.id.c_str(), tile.zoom, tile.x, tile.y,
                  FormatFileTimestamp(timestamp).c_str());
  return AllocatedPath::Build(cache_path, filename);
}

void
SkysightAPI::EnsureTile(const SkySight::Layer &layer, time_t timestamp,
                        const GeoBitmap::TileData &tile)
{
  request->DownloadFile(MakeTileUrl(layer, timestamp, tile),
                        GetTilePath(layer, timestamp, tile),
                        layer.requires_auth);
}

void
SkysightAPI::OnAuthenticated() noexcept
{
  owner.OnDataUpdated();
}

void
SkysightAPI::OnDownloadComplete() noexcept
{
  owner.OnDataUpdated();
}