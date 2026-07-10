// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The XCSoar Project

#include "SkySightAPI.hpp"
#include "SkySightRequest.hpp"
#include "SkySightClient.hpp"
#include "time/Convert.hxx"
#include "util/StaticString.hxx"

#include <chrono>
#include <ctime>

SkySightAPI::SkySightAPI(SkySightClient &_owner, CurlGlobal &curl, Path _cache_path)
  :owner(_owner),
   request(std::make_unique<SkySightRequest>(*this, curl)),
   cache_path(_cache_path)
{
  InitialiseLayers(layers);
}

SkySightAPI::~SkySightAPI() = default;

void
SkySightAPI::InitialiseLayers(std::vector<SkySight::Layer> &new_layers)
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
SkySightAPI::Configure(std::string_view email, std::string_view password,
                       std::string_view new_region)
{
  region = new_region.empty() ? "EUROPE" : std::string{new_region};
  request->Configure(email, password);
}

bool
SkySightAPI::HasCredentials() const noexcept
{
  return request->HasCredentials();
}

const SkySight::Layer *
SkySightAPI::GetLayer(std::size_t index) const noexcept
{
  return index < layers.size()
    ? &layers[index]
    : nullptr;
}

SkySight::Layer *
SkySightAPI::GetLayer(std::string_view id) noexcept
{
  for (auto &i : layers)
    if (i == id)
      return &i;

  return nullptr;
}

std::string
SkySightAPI::FormatUrlTimestamp(time_t timestamp)
{
  const auto tm = GmTime(std::chrono::system_clock::from_time_t(timestamp));

  char buffer[32];
  std::strftime(buffer, sizeof(buffer), "%Y/%m/%d/%H%M", &tm);
  return buffer;
}

std::string
SkySightAPI::FormatFileTimestamp(time_t timestamp)
{
  const auto tm = GmTime(std::chrono::system_clock::from_time_t(timestamp));

  char buffer[32];
  std::strftime(buffer, sizeof(buffer), "%Y-%m-%d-%H%M", &tm);
  return buffer;
}

std::string
SkySightAPI::MakeTileUrl(const SkySight::Layer &layer,
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
SkySightAPI::GetTilePath(const SkySight::Layer &layer, time_t timestamp,
                         const GeoBitmap::TileData &tile) const
{
  StaticString<128> filename;
  filename.Format("%s-%u-%u-%u-%s.jpg",
                  layer.id.c_str(), tile.zoom, tile.x, tile.y,
                  FormatFileTimestamp(timestamp).c_str());
  return AllocatedPath::Build(cache_path, filename);
}

void
SkySightAPI::EnsureTile(const SkySight::Layer &layer, time_t timestamp,
                        const GeoBitmap::TileData &tile)
{
  request->DownloadFile(MakeTileUrl(layer, timestamp, tile),
                        GetTilePath(layer, timestamp, tile),
                        layer.requires_auth);
}

void
SkySightAPI::OnAuthenticated() noexcept
{
  owner.OnDataUpdated();
}

void
SkySightAPI::OnDownloadComplete() noexcept
{
  owner.OnDataUpdated();
}