// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The XCSoar Project

#pragma once

#include "MapWindow/Overlay.hpp"
#include "XCThermGeoJSON.hpp"
#include "ui/canvas/Color.hpp"

#include <cstddef>
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

  /** Human-readable label (the altitude layer, e.g. "5000m AMSL") */
  std::string label;

  /**
   * The API parameter this overlay was built from (e.g.
   * "vertical_wind_5000amsl") and the forecast UTC hour it represents.
   * Used by FormatPointInfo to look up the download time / run from
   * the XCThermAPI cache. Empty / 0 when not set.
   */
  std::string parameter;
  unsigned forecast_utc = 0;

public:
  XCThermGeoJSONOverlay() noexcept = default;

  /**
   * Replace the forecast data.
   * Can be called from any thread.
   *
   * @param _label    altitude layer label shown to the user
   * @param _parameter API parameter name (for cache metadata lookup)
   * @param _forecast_utc the forecast's valid UTC hour
   */
  void SetForecast(XCThermGeoJSON::ForecastLayer &&_forecast,
                   const char *_label,
                   const char *_parameter = nullptr,
                   unsigned _forecast_utc = 0) noexcept;

  /** Is any data loaded? */
  bool HasData() const noexcept;

  [[gnu::pure]]
  const std::string &GetParameter() const noexcept;

  [[gnu::pure]]
  unsigned GetForecastUtc() const noexcept;

  /**
   * Find the vertical-wind band whose polygons contain @p p and
   * return its [min,max] m/s range. When the point falls inside
   * several nested bands, the most extreme (largest |mid|) wins —
   * that's the innermost contour and the true local value.
   *
   * @return true if a band contains the point.
   *
   * NOT [[gnu::pure]] — it writes results through the reference
   * out-parameters, which a pure function may not do (the optimizer
   * would assume the writes never happen and use the caller's
   * uninitialised/initial values instead).
   */
  bool GetClimbAt(GeoPoint p, double &out_min_ms,
                  double &out_max_ms) const noexcept;

  struct ForecastSnapshot {
    XCThermGeoJSON::ForecastLayer forecast;
    std::string parameter;
    unsigned forecast_utc = 0;
  };

  /** Thread-safe copy of the loaded forecast for off-map rendering. */
  bool TryCopyForecast(ForecastSnapshot &out) const noexcept;

  /**
   * Map vertical wind (m/s) to the XCTherm color scale.
   */
  static Color WindToColor(double min_ms, double max_ms) noexcept;

  /**
   * Wind band fill color with map-overlay alpha ramp.
   */
  static Color BandFillColor(double min_ms, double max_ms) noexcept;

  /* virtual methods from class MapOverlay */
  const char *GetLabel() const noexcept override;
  bool IsInside(GeoPoint p) const noexcept override;
  bool FormatPointInfo(GeoPoint p, char *buffer,
                       std::size_t size) const noexcept override;
  void Draw(Canvas &canvas,
            const WindowProjection &projection) noexcept override;
};
