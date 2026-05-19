// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The XCSoar Project

#pragma once

#include "MapWindow/Overlay.hpp"
#include "XCThermGeoJSON.hpp"
#include "ui/canvas/Color.hpp"

#include <memory>
#include <mutex>
#include <string>

/**
 * A MapOverlay that renders XCTherm vertical wind forecast
 * polygons parsed from GeoJSON data.
 *
 * Thread safety: The forecast data can be updated from a
 * background thread while the draw thread reads it. A mutex
 * protects the shared state.
 */
class XCThermGeoJSONOverlay final : public MapOverlay {
  mutable std::mutex mutex;

  /** The parsed forecast data (protected by mutex) */
  XCThermGeoJSON::ForecastLayer forecast;

  /** Human-readable label */
  std::string label;

public:
  XCThermGeoJSONOverlay() noexcept = default;

  /**
   * Replace the forecast data.
   * Can be called from any thread.
   */
  void SetForecast(XCThermGeoJSON::ForecastLayer &&_forecast,
                   const char *_label) noexcept;

  /** Is any data loaded? */
  bool HasData() const noexcept;

  /* virtual methods from class MapOverlay */
  const char *GetLabel() const noexcept override;
  bool IsInside(GeoPoint p) const noexcept override;
  void Draw(Canvas &canvas,
            const WindowProjection &projection) noexcept override;

private:
  /**
   * Map a vertical wind speed (m/s) to an RGBA color.
   * Uses the standard XCTherm color scale:
   *   strong sink = blue, weak sink = light blue,
   *   weak lift = light red/orange, strong lift = red
   */
  static Color WindToColor(double min_ms, double max_ms) noexcept;
};
