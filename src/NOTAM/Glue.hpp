// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The XCSoar Project

#pragma once

#include "Settings.hpp"
#include "NOTAM.hpp"
#include "co/InjectTask.hxx"
#include "thread/Mutex.hxx"
#include "Geo/GeoPoint.hpp"
#include <vector>
#include <memory>

class CurlGlobal;
struct NOTAM; // Explicit forward declaration
class Airspaces;
class OperationEnvironment;
class AllocatedPath;

/**
 * NOTAM manager that handles loading and refreshing NOTAMs
 */
class NOTAMGlue {
  const NOTAMSettings &settings;
  CurlGlobal &curl;
  
  mutable Mutex mutex;
  
  /** Current location for NOTAM searches */
  GeoPoint current_location = GeoPoint::Invalid();
  
  /** Currently loaded NOTAMs - using void* to avoid template issues */
  void* current_notams_impl;
  
  /** Background task for loading NOTAMs */
  Co::InjectTask load_task;
  
  /** Whether NOTAMs are currently being loaded */
  bool loading = false;

public:
  NOTAMGlue(const NOTAMSettings &_settings, CurlGlobal &_curl);
  ~NOTAMGlue();

  /**
   * Update the current location and trigger NOTAM refresh if needed
   */
  void UpdateLocation(const GeoPoint &location);
  
  /**
   * Load NOTAMs for the given location
   */
  void LoadNOTAMs(const GeoPoint &location, OperationEnvironment &operation);
  
  /**
   * Get currently loaded NOTAMs (thread-safe)
   * Returns number of NOTAMs copied to the provided buffer
   */
  unsigned GetNOTAMs(void* buffer, unsigned max_count) const;
  
  /**
   * Clear all loaded NOTAMs
   */
  void Clear();
  
  /**
   * Check if NOTAMs are currently being loaded
   */
  bool IsLoading() const {
    const std::lock_guard<Mutex> lock(mutex);
    return loading;
  }

  /**
   * Test function to verify NOTAM functionality
   * @param location Test location (e.g., airport coordinates)
   * @return Number of NOTAMs found, or -1 on error
   */
  int TestNOTAMFetch(const GeoPoint &location);

  /** Generate cache file path for NOTAMs at a specific location */
  AllocatedPath GetNOTAMCacheFilePath(const GeoPoint &location) const;

  /**
   * Load NOTAMs from cache and return count
   * @param location The location to load NOTAMs for
   * @return Number of NOTAMs loaded from cache
   */
  unsigned LoadCachedNOTAMs(const GeoPoint &location);

  /**
   * Update the airspace database with NOTAM data
   * @param airspaces The airspace database to update
   */
  void UpdateAirspaces(Airspaces &airspaces);

private:
  Co::InvokeTask LoadNOTAMsInternal(GeoPoint location);
  void OnLoadComplete(std::exception_ptr error) noexcept;
  
  /** Save raw GeoJSON response to file */
  void SaveNOTAMsToFile(const GeoPoint &location, const std::string &geojson_response) const;
  
  /** Load NOTAMs from cache file if available */
  bool LoadNOTAMsFromFile(const GeoPoint &location, std::vector<struct NOTAM> &notams) const;
  
  /** Check if cached data is expired based on settings */
  bool IsCacheExpired(const GeoPoint &location) const;
};