// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The XCSoar Project

#include "XCThermMapOverlay.hpp"
#include "XCThermGeoJSONOverlay.hpp"
#include "LogFile.hpp"
#include "MapWindow/GlueMapWindow.hpp"
#include "UIGlobals.hpp"

namespace XCTherm {

void
ApplyForecastToMap(const std::string &geojson, const char *label,
                   const char *parameter, const unsigned forecast_utc) noexcept
{
  auto forecast = XCThermGeoJSON::Parse(geojson, true);
  if (forecast.IsEmpty()) {
    LogFmt("xctherm: parse failed for {}", label);
    return;
  }

  forecast.layer_name = label;
  ApplyForecastLayerToMap(std::move(forecast), label, parameter, forecast_utc);
}

void
ApplyForecastLayerToMap(XCThermGeoJSON::ForecastLayer &&forecast,
                        const char *label, const char *parameter,
                        const unsigned forecast_utc) noexcept
{
  if (forecast.IsEmpty())
    return;

  auto *map = UIGlobals::GetMap();
  if (map == nullptr)
    return;

  auto overlay = std::make_unique<XCThermGeoJSONOverlay>();
  overlay->SetForecast(std::move(forecast), label, parameter, forecast_utc);
  map->SetOverlay(std::move(overlay));
}

void
ClearMapOverlay() noexcept
{
  auto *map = UIGlobals::GetMap();
  if (map != nullptr)
    map->SetOverlay(nullptr);
}

} // namespace XCTherm
