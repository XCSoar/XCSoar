// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The XCSoar Project

#pragma once

#include "Settings.hpp"
#include "NOTAM.hpp"
#include "co/InjectTask.hxx"
#include "thread/Mutex.hxx"
#include "ui/event/Notify.hpp"
#include "Geo/GeoPoint.hpp"
#include "system/Path.hpp"
#include "RateLimiter.hpp"
#include <boost/json/fwd.hpp>
#include <chrono>
#include <cstdint>
#include <ctime>
#include <memory>
#include <optional>
#include <string>
#include <vector>

class CurlGlobal;
class Airspaces;
struct NOTAMImpl; // Forward declaration for pimpl

/**
 * Listener interface for NOTAM updates
 */
class NOTAMListener {
public:
  virtual ~NOTAMListener() = default;
  
  /**
   * Called when NOTAMs have been successfully loaded and filtered
   */
  virtual void OnNOTAMsUpdated() noexcept = 0;

  /**
   * Called when a NOTAM load attempt has completed.
   */
  virtual void OnNOTAMsLoadComplete(bool success) noexcept = 0;
};

/**
 * NOTAM manager that handles loading and refreshing NOTAMs
 */
class NOTAMGlue final : public RateLimiter {
  NOTAMSettings settings;
  CurlGlobal &curl;
  
  mutable Mutex mutex;
  
  /** Current location for NOTAM searches */
  GeoPoint current_location = GeoPoint::Invalid();

  /** Latest UTC time from the main-thread blackboard/timer path. */
  std::chrono::system_clock::time_point current_time_utc{};
  
  /** Currently loaded NOTAMs (pimpl pattern for encapsulation) */
  std::unique_ptr<NOTAMImpl> current_notams_impl;
  
  /** Whether NOTAMs are currently being loaded */
  bool loading = false;
  
  /** Whether a retry is pending (waiting for RateLimiter timer) */
  bool retry_pending = false;
  
  /**
   * Timestamp of last fetch attempt (success or failure) to prevent rapid
   * retries.
   */
  std::time_t last_attempt_time = 0;
  
  /** Registered listeners for NOTAM updates */
  std::vector<NOTAMListener *> listeners;

  /** Marshals listener callbacks from worker paths onto the main thread. */
  UI::Notify listener_notify{[this]{ OnListenerNotification(); }};

  bool listener_update_pending = false;
  std::optional<bool> listener_load_complete_pending;
  bool last_load_committed = false;

  /** True after a fetch-failure message has been shown. */
  bool fetch_failure_notified = false;

  /** Set by manual refresh requests to allow one user-facing success message. */
  bool manual_refresh_requested = false;

  /** Incremented when data is explicitly cleared/invalidated. */
  uint64_t mutation_generation = 0;

  /** Background task for loading NOTAMs.  Declared last so destroyed first. */
  Co::InjectTask load_task;

public:
  NOTAMGlue(const NOTAMSettings &_settings, CurlGlobal &_curl);
  ~NOTAMGlue();
  
  /**
   * Register a listener for NOTAM update notifications
   */
  void AddListener(NOTAMListener &listener);
  
  /**
   * Unregister a listener
   */
  void RemoveListener(NOTAMListener &listener) noexcept;

  /**
   * Allow the next fetch failure to show a user-facing notification.
   */
  void ResetFetchFailureNotification() noexcept;

  /** Mark the next successful load as user-initiated. */
  void MarkManualRefreshRequested() noexcept;

  void SetSettings(const NOTAMSettings &_settings) noexcept;

  /**
   * Update the current location and trigger NOTAM refresh if needed
   */
  void UpdateLocation(const GeoPoint &location);

  /**
   * Force a network fetch for the current location by bypassing cache checks.
   *
   * @param invalidate_cache_state Clear cached API state before starting the
   * fetch. Existing in-memory NOTAMs are kept until the new response commits.
   * @return True if a load request was started.
   */
  bool ForceUpdateLocation(const GeoPoint &location,
                           bool invalidate_cache_state = false);
  
  /**
   * Periodic timer callback - checks if NOTAMs need to be refreshed
   * based on time interval and location change.
   * Called from ProcessTimer.
   */
  void OnTimer(const GeoPoint &current_location,
               std::chrono::system_clock::time_point current_time_utc);
  
  /**
   * Load NOTAMs for the given location
   */
  void LoadNOTAMs(const GeoPoint &location);
  
  /**
   * Get currently loaded NOTAMs (thread-safe)
   * Returns a copy of the NOTAMs, limited to max_count if specified
   * (0 = no limit).
   */
  std::vector<struct NOTAM> GetNOTAMs(unsigned max_count = 0) const;

  struct Snapshot {
    std::vector<struct NOTAM> notams;
    std::time_t last_update_time = 0;
    GeoPoint last_update_location = GeoPoint::Invalid();
  };

  /**
   * Get a consistent snapshot of NOTAMs and fetch metadata.
   */
  [[nodiscard]] Snapshot GetSnapshot() const;
  
  /**
   * Clear all loaded NOTAMs
   */
  void Clear();
  
  /**
   * Check if NOTAMs are currently being loaded
   */
  [[nodiscard]] bool IsLoading() const {
    const std::lock_guard<Mutex> lock(mutex);
    return loading;
  }

  /** Generate cache file path for NOTAMs */
  AllocatedPath GetNOTAMCacheFilePath() const;

  /**
   * Load NOTAMs from cache and return count
   * @return Number of NOTAMs loaded from cache
   */
  unsigned LoadCachedNOTAMs();

  /**
   * Load cached NOTAMs and update airspaces if cache is available.
   * Startup helper: caller must hold ScopeSuspendAllThreads while calling
   * this method before touching the shared airspace database.
   * If @p current_location is valid, cached NOTAMs are only applied when the
   * cache coverage still contains that location.
   * @return True if the cache was loaded and applied.
   */
  bool LoadCachedNOTAMsAndUpdate(Airspaces &airspaces,
                                 const GeoPoint &current_location =
                                   GeoPoint::Invalid());

  /**
   * Update the airspace database with NOTAM data
   * @param airspaces The airspace database to update
   */
  void UpdateAirspaces(Airspaces &airspaces);

  /**
   * Get the timestamp of the last NOTAM update from cache file
   * @return Timestamp in seconds since epoch, or 0 if no cache exists
   */
  [[nodiscard]] std::time_t GetLastUpdateTime() const;

  /**
   * Get the location (latitude/longitude) that was used for the last NOTAM
   * fetch as stored in the cache file metadata.
   * @return GeoPoint of last update, or Invalid() if not available
   */
  [[nodiscard]] GeoPoint GetLastUpdateLocation() const;

  /**
   * Invalidate the NOTAM cache by deleting the cache file
   * This will force a fresh fetch on the next update
   */
  void InvalidateCache();

  /**
   * Statistics about filter results
   */
  struct FilterStats {
    unsigned total = 0;              // Total NOTAMs loaded
    unsigned filtered_by_ifr = 0;    // Count filtered by IFR filter
    unsigned filtered_by_time = 0;   // Count filtered by time filter
    unsigned filtered_by_qcode = 0;  // Count filtered by Q-code filter
    unsigned filtered_by_radius = 0; // Count filtered by radius filter
    unsigned final_count = 0;        // Final count after all filters
  };

  /**
   * Get the total number of currently loaded NOTAMs
   */
  [[nodiscard]] unsigned GetTotalCount() const;

  /**
   * Get the number of NOTAMs that pass the current filters
   */
  [[nodiscard]] unsigned GetFilteredCount() const;

  /**
   * Get detailed filter statistics showing how many NOTAMs each filter removes
   */
  [[nodiscard]] FilterStats GetFilterStats() const;

  /**
   * Find a NOTAM by its number (e.g., "A1234/24")
   * @param number The NOTAM number to search for
   * @return NOTAM if found, std::nullopt otherwise
   */
  [[nodiscard]] std::optional<struct NOTAM>
  FindNOTAMByNumber(const std::string &number) const;

private:
  [[nodiscard]] NOTAMSettings GetSettingsSnapshot() const noexcept;
  [[nodiscard]] std::chrono::system_clock::time_point
  GetCurrentTimeUTCSnapshot() const noexcept;
  Co::InvokeTask LoadNOTAMsInternal(GeoPoint location, bool force_network);
  void OnLoadComplete(std::exception_ptr error) noexcept;
  void OnListenerNotification() noexcept;
  
  /** Notify all registered listeners that NOTAMs have been updated */
  void NotifyListeners() noexcept;
  
  /** RateLimiter callback for delayed retry */
  void Run() override;
  
  /** Save raw API response to file */
  void SaveNOTAMsToFile(const boost::json::value &api_response,
                        const GeoPoint &location,
                        uint64_t expected_generation) const;
  
  /**
   * Load NOTAMs from cache file if available
   * @param notams Vector to store loaded NOTAMs
   * @param location Optional output for the location from cache metadata
   * @param radius_km Optional output for the radius from cache metadata
   * @param api_response Optional output for the raw API response from cache
   * @return True if cache was loaded successfully, false otherwise
   */
  bool LoadNOTAMsFromFile(std::vector<struct NOTAM> &notams,
                          GeoPoint *location = nullptr,
                          unsigned *radius_km = nullptr,
                          boost::json::value *api_response = nullptr) const;
  
  /** Check if cached data is expired based on settings */
  bool IsCacheExpired() const;

  /**
   * Get the radius (km) that was used for the last NOTAM fetch
   * as stored in the cache file metadata.
   * @return Radius in kilometers, or 0 if not available
   */
  [[nodiscard]] unsigned GetLastUpdateRadius() const;
};
