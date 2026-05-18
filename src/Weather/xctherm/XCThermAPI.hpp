// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The XCSoar Project

#pragma once

#include "XCThermAuth.hpp"
#include "Operation/Operation.hpp"

#include <cstdint>
#include <cstddef>
#include <map>
#include <string>
#include <vector>

/**
 * XCTherm GeoJSON API client.
 *
 * Fetches index.json to discover available parameters and forecast times,
 * then downloads individual GeoJSON layers on demand.
 *
 * URL schema:
 *   Index:    https://tiles.xctherm.com/forecast/{model}/index.json
 *   GeoJSON:  https://tiles.xctherm.com/forecast/{model}/{YYYYMMDD}/{HH}/{STEP}/{parameter}.geojson
 */
class XCThermAPI {
public:
  /**
   * A single forecast slot from index.json.
   * Forecast time = run_datetime + step hours.
   */
  struct ForecastSlot {
    std::string run_date;   // "20260506"
    std::string run_hour;   // "03"
    unsigned step_min = 0;
    unsigned step_max = 0;
    unsigned step_step = 1;
  };

  /**
   * Info about one available parameter (e.g. "vertical_wind_3000amsl").
   */
  struct ParameterInfo {
    std::string name;
    std::vector<ForecastSlot> slots;
  };

  static XCThermAPI &Instance();

  XCThermAPI();
  ~XCThermAPI();

  XCThermAPI(const XCThermAPI &) = delete;
  XCThermAPI &operator=(const XCThermAPI &) = delete;

  void SetCredentials(const std::string &email,
                      const std::string &password) noexcept;
  void SetModel(const std::string &model) noexcept;

  const std::string &GetModel() const noexcept { return model; }

  /**
   * Fetch and parse index.json for the current model.
   * Populates available_parameters.
   * Returns true on success.
   */
  bool FetchIndex() noexcept;

  /**
   * Get available vertical_wind parameters from the last FetchIndex().
   */
  const std::vector<ParameterInfo> &GetAvailableParameters() const noexcept {
    return available_parameters;
  }

  /**
   * Get available forecast UTC hours for a given parameter.
   * Returns sorted unique hours.
   */
  std::vector<unsigned> GetAvailableForecastHours(
    const std::string &parameter) const noexcept;

  /**
   * Find the best slot (run + step) for a given parameter and target UTC hour.
   * Prefers the most recent run that covers the target hour.
   *
   * @param parameter e.g. "vertical_wind_3000amsl"
   * @param target_utc_hour 0-23
   * @param out_date filled with YYYYMMDD
   * @param out_run_hour filled with HH
   * @param out_step filled with step number
   * @return true if a valid slot was found
   */
  bool FindSlotForHour(const std::string &parameter,
                       unsigned target_utc_hour,
                       std::string &out_date,
                       std::string &out_run_hour,
                       unsigned &out_step) const noexcept;

  /**
   * Find the best slot for a forecast N hours into the future.
   *
   * Unlike FindSlotForHour, this handles the offset correctly across
   * midnight by picking the slot whose (run_h + step) equals
   * (current_utc + offset_hours), preferring the highest step (=most
   * future forecast) from the latest available run.
   *
   * @param parameter e.g. "vertical_wind_3000amsl"
   * @param current_utc_hour current UTC hour (0-23)
   * @param offset_hours forecast offset (1, 6, 12, 18)
   * @param out_date filled with YYYYMMDD
   * @param out_run_hour filled with HH
   * @param out_step filled with step number
   * @return true if a valid slot was found
   */
  bool FindSlotForOffset(const std::string &parameter,
                         unsigned current_utc_hour,
                         unsigned offset_hours,
                         std::string &out_date,
                         std::string &out_run_hour,
                         unsigned &out_step) const noexcept;

  /**
   * Download a single GeoJSON layer.
   *
   * @param parameter e.g. "vertical_wind_3000amsl"
   * @param date YYYYMMDD
   * @param run_hour HH (00, 03, etc.)
   * @param step forecast step (0, 1, 2, ...)
   * @param out_geojson receives the uncompressed GeoJSON string
   * @param env optional progress reporting (SetText + SetProgressRange/Position)
   * @return true on success
   */
  bool DownloadGeoJSON(const std::string &parameter,
                       const std::string &date,
                       const std::string &run_hour,
                       unsigned step,
                       std::string &out_geojson,
                       OperationEnvironment *env = nullptr,
                       int64_t *out_wire_bytes = nullptr) noexcept;

  /**
   * Convenience: download a layer for a target UTC hour.
   * Calls FindSlotForHour + DownloadGeoJSON.
   */
  bool DownloadLayerForHour(const std::string &parameter,
                            unsigned target_utc_hour,
                            std::string &out_geojson,
                            OperationEnvironment *env = nullptr) noexcept;

  bool IsIndexLoaded() const noexcept { return index_loaded; }

  /* ---- Download cache ---- */

  /**
   * Check if a layer has already been downloaded for the given UTC hour.
   */
  bool IsLayerCached(const std::string &parameter,
                     unsigned utc_hour) const noexcept;

  /**
   * Get cached GeoJSON for a layer/hour combination.
   * Returns empty string if not cached.
   */
  const std::string &GetCachedGeoJSON(const std::string &parameter,
                                       unsigned utc_hour) const noexcept;

  /**
   * Get all UTC hours that are cached for a given parameter.
   * Returns sorted list.
   */
  std::vector<unsigned> GetCachedHours(const std::string &parameter) const noexcept;

  /**
   * Get all parameters that have at least one cached entry.
   */
  std::vector<std::string> GetCachedParameters() const noexcept;

  /**
   * Clear the entire download cache.
   */
  void ClearCache() noexcept;

private:
  XCThermAuth auth;
  std::string model = "icon-ch";
  std::vector<ParameterInfo> available_parameters;
  bool index_loaded = false;

  /** Cache: parameter -> (utc_hour -> geojson) */
  std::map<std::string, std::map<unsigned, std::string>> geojson_cache;

  static const std::string kEmptyString;

  /**
   * Parse the index.json response and populate available_parameters.
   */
  bool ParseIndex(const std::string &json) noexcept;

  /**
   * Format step as 3-digit string (e.g. 9 → "009").
   */
  static std::string FormatStep(unsigned step) noexcept;
};
