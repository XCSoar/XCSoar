// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The XCSoar Project

#pragma once

#include "co/Task.hxx"
#include "net/client/xctherm/Auth.hpp"
#include "Operation/Operation.hpp"

#include "system/Path.hpp"

class CurlGlobal;

#include <cstdint>
#include <cstddef>
#include <functional>
#include <list>
#include <map>
#include <mutex>
#include <optional>
#include <stdexcept>
#include <string>
#include <vector>

class ProgressListener;
struct XCThermSettings;

/**
 * Errors thrown by XCThermAPI on network / HTTP failures. Callers can
 * inspect @c kind to decide whether to retry (transient: NETWORK,
 * SERVER_ERROR, NOT_FOUND for a single slot) or surface to the user
 * (persistent: AUTH_FAILED, FORBIDDEN, OTHER_HTTP).
 *
 * what() returns a short human-readable explanation already suitable
 * for ShowError.
 */
class XCThermAPIError : public std::runtime_error {
public:
  enum class Kind {
    NETWORK,        ///< curl-level transport failure (timeout, DNS, etc.)
    AUTH_FAILED,    ///< 401 after re-auth attempt
    FORBIDDEN,      ///< 403
    NOT_FOUND,      ///< 404 — a single slot might genuinely be absent
    SERVER_ERROR,   ///< 5xx
    OTHER_HTTP,     ///< unexpected HTTP code
  };

  XCThermAPIError(Kind kind, long http_code, std::string message) noexcept
    : std::runtime_error(std::move(message)),
      kind(kind), http_code(http_code) {}

  Kind kind;
  long http_code;
};

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

  /**
   * Tiles URL model slug (e.g. @c "icon-ch"), not @c settings.xctherm.model.
   */
  void SetModel(const std::string &api_slug) noexcept;

  /**
   * Map profile region id (@c XCTherm::Region / settings.xctherm.model)
   * to the tiles API slug and invalidate the index when it changes.
   */
  void SetRegion(unsigned region_model_id) noexcept;

  /** Sync credentials and region from computer settings. */
  void ApplySessionSettings(const XCThermSettings &settings) noexcept;

  /** Enable disk cache and apply @p settings (idempotent). */
  void PrepareSession(const XCThermSettings &settings) noexcept;

  /**
   * Fetch and parse index.json for the current model.
   * Populates available_parameters.
   *
   * @throws XCThermAPIError on network / HTTP failure.
   * @return true on success, false only if parsing yielded no
   *   recognisable forecast parameters (server response not in the
   *   expected shape — treated as a "no XCTherm data" non-error).
   */
  bool FetchIndex();

  /**
   * Fetch index.json on @p curl's event loop when not already loaded.
   *
   * @throws XCThermAPIError on network / HTTP failure.
   * @return false if the response contained no forecast parameters.
   */
  Co::Task<bool> CoEnsureIndexLoaded(CurlGlobal &curl);

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
   * Download a single GeoJSON layer on @p curl's event loop (async).
   *
   * Ensures a valid JWT via #auth before the request; sends Bearer auth;
   * on HTTP 401 re-authenticates once and retries.
   *
   * @return @c true on success; @c false if @p should_continue returned false.
   * @throws XCThermAPIError on network / HTTP failure.
   */
  Co::Task<bool> CoDownloadGeoJSON(CurlGlobal &curl,
                                   const std::string &parameter,
                                   const std::string &date,
                                   const std::string &run_hour,
                                   unsigned step,
                                   std::string &out_geojson,
                                   int64_t *out_wire_bytes = nullptr,
                                   ProgressListener *progress = nullptr,
                                   const std::function<bool()> &should_continue = []() {
                                     return true;
                                   });

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

    /**
     * Wall-clock moment this slice was first written to the cache
     * (seconds since the Unix epoch). Persisted in the on-disk header
     * so it survives across app restarts and is shown by the dialog
     * to indicate forecast freshness. Zero for slices loaded from an
     * older (#XCTHERMv1) file with no embedded timestamp — callers
     * should fall back to the file mtime in that case.
     */
    int64_t downloaded_at = 0;
  };

  /**
   * Lightweight metadata about a cached slice — everything except the
   * (potentially multi-MB) GeoJSON body. The disk index holds one of
   * these per cached slice so we can answer "what's available, from
   * which run, downloaded when" without loading any bodies into RAM.
   */
  struct SliceMeta {
    std::string run_date;
    std::string run_hour;
    unsigned step = 0;
    int64_t downloaded_at = 0;
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
   * Get cached GeoJSON for a layer/hour combination, returned by value.
   * If the slice exists on disk but isn't resident in RAM, it is
   * faulted in (read from disk) and kept in a small LRU body cache.
   * Returns an empty string if the slice isn't cached at all.
   *
   * Not const: it mutates the in-RAM LRU body cache. Call from the UI
   * or download-worker thread, never from the map draw thread.
   */
  std::string GetCachedGeoJSON(const std::string &parameter,
                               unsigned utc_hour) noexcept;

  /**
   * Get the model run / freshness metadata for a cached slice, read
   * from the disk index (no body load). std::nullopt if not cached.
   */
  std::optional<SliceMeta> GetSliceMeta(const std::string &parameter,
                                        unsigned utc_hour) const noexcept;

  /**
   * Get all UTC hours that are cached for a given parameter.
   * Returns sorted list.
   */
  std::vector<unsigned> GetCachedHours(const std::string &parameter) const noexcept;

  /**
   * Summary of what's cached for a single forecast parameter — used by
   * the XCTherm dialog on open to rehydrate per-row state from the
   * persistent disk cache, without having to lock and look up every
   * slice individually.
   *
   * All fields are empty / zero when nothing is cached for the
   * parameter.
   */
  struct LayerCacheSummary {
    std::vector<unsigned> hours;          ///< sorted cached UTC hours
    int64_t latest_downloaded_at = 0;     ///< newest write time (unix s)
    std::string latest_run_date;          ///< run_date of the newest slice
    std::string latest_run_hour;          ///< run_hour of the newest slice

    /**
     * How many of the cached slices are still valid in the future,
     * computed at query time against wall-clock. A slice's "valid
     * time" is run_date + run_hour + step. Used by the dialog to
     * show "4 / 12 h" — meaning 4 cached future-hours remain of the
     * 12 h span the user has configured.
     */
    unsigned future_hours = 0;
  };

  LayerCacheSummary GetCachedLayerSummary(
      const std::string &parameter) const noexcept;

  /**
   * Quick "is there any cached forecast at all?" check, used by the
   * controls widget to collapse itself to a sliver when no XCTherm
   * data has been downloaded — so non-XCTherm users don't see an
   * empty cursor bar.
   */
  bool HasAnyCache() const noexcept;

  /**
   * Initialise the persistent disk cache directory and load any
   * existing files from previous sessions. Files older than the TTL
   * (24 h) are discarded on load.
   *
   * Idempotent — safe to call repeatedly; only the first call does
   * work. Call once at app startup from a UI-thread context.
   */
  void EnableDiskCache() noexcept;

  /**
   * Clear every cached entry for a single parameter (e.g. one altitude
   * layer). Used by the dialog's Delete button.
   */
  void ClearLayer(const std::string &parameter) noexcept;

  /**
   * Stale-run prune: drop cached slices for @p parameter whose
   * (run_date, run_hour) is older than the latest run the index
   * currently advertises for that UTC hour.
   *
   * Only the cached entries are touched; nothing is re-downloaded.
   * Slices for past hours are kept (the dialog lets the user browse
   * back), but their run version is harmonised with the rest.
   *
   * Returns the number of entries removed.
   */
  unsigned PruneStaleRuns(const std::string &parameter,
                          unsigned current_utc_hour) noexcept;

private:
  XCTherm::Auth auth{XCTherm::kAuthConfig};

  /** Tiles path segment; kept in sync via #SetRegion / #ApplySessionSettings. */
  std::string model = "icon-ch";
  std::vector<ParameterInfo> available_parameters;
  bool index_loaded = false;

  /**
   * Disk index: parameter -> (utc_hour -> SliceMeta). The authoritative
   * record of what is cached on disk, populated by EnableDiskCache()
   * scanning file headers (cheap) and kept in sync by the write paths.
   * Metadata queries (hours, run, freshness) answer from here without
   * touching any GeoJSON body — that's what keeps startup RAM low.
   */
  std::map<std::string, std::map<unsigned, SliceMeta>> disk_index;

  /**
   * In-RAM body cache: parameter -> (utc_hour -> CachedSlice with the
   * GeoJSON body). A *bounded* LRU subset of what's in disk_index —
   * only slices recently accessed via GetCachedGeoJSON are resident.
   * Capped at kMaxResidentSlices so we never hold the whole 24 h × N
   * layer cache (hundreds of MB) in memory at once.
   */
  std::map<std::string, std::map<unsigned, CachedSlice>> geojson_cache;

  /** MRU-ordered keys for geojson_cache; front = most recently used. */
  std::list<std::pair<std::string, unsigned>> lru_order;

  /** Max number of GeoJSON bodies kept resident in geojson_cache. */
  static constexpr std::size_t kMaxResidentSlices = 12;

  /** Guards disk_index, geojson_cache and lru_order. */
  mutable std::mutex cache_mutex;

  /**
   * Absolute path to the persistent cache directory (e.g.
   * …/XCSoarData/weather/xctherm). Empty when disk caching is not
   * (yet) enabled.
   */
  AllocatedPath disk_cache_dir = nullptr;

  /* ---- Disk cache helpers ---- */

  /**
   * Compute the path of the on-disk cache file for one slice.
   * Returns nullptr if disk_cache_dir is not set.
   */
  AllocatedPath SliceFilePath(const std::string &parameter,
                              unsigned forecast_utc) const noexcept;

  /** Write the slice to disk. No-op if disk cache disabled. */
  void WriteSliceToDisk(const std::string &parameter,
                        unsigned forecast_utc,
                        const CachedSlice &slice) const noexcept;

  /** Delete the on-disk file for one slice. No-op if disk cache disabled. */
  void DeleteSliceFromDisk(const std::string &parameter,
                           unsigned forecast_utc) const noexcept;

  /**
   * Read and parse one on-disk slice file (header + GeoJSON body).
   * @return true on success, with @p out fully populated.
   */
  bool ReadSliceFile(const std::string &parameter, unsigned utc_hour,
                     CachedSlice &out) const noexcept;

  /**
   * Insert a freshly-obtained body into the LRU body cache, recording
   * it as most-recently-used and evicting the least-recently-used
   * entries beyond kMaxResidentSlices. Caller must hold cache_mutex.
   */
  void InsertResident(const std::string &parameter, unsigned utc_hour,
                      CachedSlice &&slice) noexcept;

  /** Mark (parameter, utc_hour) most-recently-used. Caller holds lock. */
  void TouchResident(const std::string &parameter,
                     unsigned utc_hour) noexcept;

  /** Drop a key from the LRU body cache. Caller holds lock. */
  void DropResident(const std::string &parameter,
                    unsigned utc_hour) noexcept;

  Co::Task<bool> CoFetchIndex(CurlGlobal &curl);

  /**
   * Parse the index.json response and populate available_parameters.
   */
  bool ParseIndex(const std::string &json) noexcept;

  /**
   * Format step as 3-digit string (e.g. 9 → "009").
   */
  static std::string FormatStep(unsigned step) noexcept;
};
