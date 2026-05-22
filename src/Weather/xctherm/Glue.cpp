// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The XCSoar Project

#include "Glue.hpp"

#include "Dialogs/Error.hpp"
#include "MapWindow/GlueMapWindow.hpp"
#include "UIGlobals.hpp"
#include "Weather/xctherm/XCThermGeoJSONOverlay.hpp"

#include <memory>
#include <stdexcept>

namespace XCTherm {

void
ApplyForecastToMap(XCThermGeoJSON::ForecastLayer &&forecast,
                   const char *label) noexcept
{
  auto *map = UIGlobals::GetMap();
  if (map == nullptr)
    return;

  try {
    if (forecast.IsEmpty())
      throw std::runtime_error("Failed to parse forecast data");

    forecast.layer_name = label != nullptr ? label : "XCTherm";

    auto overlay = std::make_unique<XCThermGeoJSONOverlay>();
    overlay->SetForecast(std::move(forecast), label);
#ifdef ENABLE_OPENGL
    map->SetOverlay(std::move(overlay));
#endif
  } catch (...) {
    ShowError(std::current_exception(), "XCTherm");
  }
}

void
ApplyForecastToMap(std::string_view geojson, const char *label) noexcept
{
  try {
    auto forecast = XCThermGeoJSON::Parse(geojson, true);
    ApplyForecastToMap(std::move(forecast), label);
  } catch (...) {
    ShowError(std::current_exception(), "XCTherm");
  }
}

} // namespace XCTherm
