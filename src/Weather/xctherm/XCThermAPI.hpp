// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The XCSoar Project

#pragma once

#include "XCThermAuth.hpp"
#include "Operation/Operation.hpp"

#include <cstdint>
#include <cstddef>
#include <functional>
#include <map>
#include <mutex>
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
   * Progress / cancel callback. Called periodically from inside
   * DownloadGeoJSON during the libcurl transfer.
   *
   * @param bytes_now   bytes received so far on the current slice
   * @param bytes_total expected total in bytes (0 if not yet known)
   * @return true to continue; false to abort the transfer
   *         (curl will return failure)
   */
  using ProgressFn = std::function<bool(uint64_t bytes_now,
                                        uint64_t bytes_total)>;

  /**
   * Download a single GeoJSON layer.
   *
   * @param parameter e.g. "vertical_wind_3000amsl"
   * @param date YYYYMMDD
   * @param run_hour HH (00, 03, etc.)
   * @param step forecast step (0, 1, 2, ...)
   * @param out_geojson receives the uncompressed GeoJSON string
   * @param progress optional progress / cancel callback. Safe to call
   *   from any thread (callback fires on the calling thread).
   * @return true on success
   */
  bool DownloadGeoJSON(const std::string &parameter,
                       const std::string &date,
                       const std::string &run_hour,
                       unsigned step,
                       std::string &out_geojson,
                       int64_t *out_wire_bytes = nullptr,
                       ProgressFn progress = nullptr) noexcept;

  bool IsIndexLoaded() const noexcept { return index_loaded; }

  /* ---- Download cache ---- */

  /**
   * One cached forecast slice.
   *
   * @c run_date / @c run_hour identify the model run that issued the
   * forecast (e.g. "20260518" / "12"). We track this so we can decide
   * whether a cached entry is still fresh when the API offers a newer
   * run for the same UTC hour.
   *
   * @c step is the forecast offset from the run (in hours). Together
   * with run_date/run_hour it pinpoints the absolute UTC moment the
   * forecast is valid for — needed because spans longer than 12 h
   * cross midnight and would otherwise be ambiguous when looked up by
   * UTC hour alone.
   */
  struct CachedSlice {
    std::string geojson;
    std::string run_date;
    std::string run_hour;
    unsigned step = 0;
  };

  /**
   * Check if a layer has already been downloaded for the given UTC hour.
   */
  bool IsLayerCached(const std::string &parameter,
                     unsigned utc_hour) const noexcept;

  /**
   * Check if the cached entry for (parameter, utc_hour) was produced by
   * the given run. Used to decide whether a Download click for an
   * already-cached hour should be skipped (cache matches latest run) or
   * re-issued (a newer run is now available).
   */
  bool IsCachedAtRun(const std::string &parameter,
                     unsigned utc_hour,
                     const std::string &run_date,
                     const std::string &run_hour) const noexcept;

  /**
   * Get cached GeoJSON for a layer/hour combination.
   * Returns empty string if not cached.
   */
  const std::string &GetCachedGeoJSON(const std::string &parameter,
                                       unsigned utc_hour) const noexcept;

  /**
   * Get the model run (date, hour) that produced the cached slice.
   * Returns nullptr if not cached.
   */
  const CachedSlice *GetCachedSlice(const std::string &parameter,
                                    unsigned utc_hour) const noexcept;

  /**
   * Get all UTC hours that are cached for a given parameter.
   * Returns sorted list.
   */
  std::vector<unsigned> GetCachedHours(const std::string &parameter) const noexcept;

  /**
   * Clear the entire download cache.
   */
  void ClearCache() noexcept;

private:
  XCThermAuth auth;
  std::string model = "icon-ch";
  std::vector<ParameterInfo> available_parameters;
  bool index_loaded = false;

  /**
   * Cache: parameter -> (utc_hour -> CachedSlice).
   *
   * Guarded by cache_mutex because the download worker thread (run from
   * the dialog) writes to it while the UI thread (controls widget,
   * dialog re-renders) reads from it. Hold only for the brief actual
   * map operation — never across a curl call.
   */
  std::map<std::string, std::map<unsigned, CachedSlice>> geojson_cache;
  mutable std::mutex cache_mutex;

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
