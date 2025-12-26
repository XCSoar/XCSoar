// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The XCSoar Project

#include "NOTAMGlue.hpp"
#include "NOTAM.hpp"
#include "Client.hpp"
#include "Components.hpp"
#include "DataComponents.hpp"
#include "Message.hpp"
#include "Airspace/Airspaces.hpp"
#include "Airspace/AirspaceCircle.hpp"
#include "Airspace/AirspacePolygon.hpp"
#include "Airspace/AirspaceClass.hpp"
#include "Airspace/AirspaceAltitude.hpp"
#include "TransponderCode.hpp"
#include "Operation/Operation.hpp"
#include "Operation/ProgressListener.hpp"
#include "io/FileOutputStream.hxx"
#include "io/FileReader.hxx"
#include "io/StringOutputStream.hxx"
#include "system/Path.hpp"
#include "system/FileUtil.hpp"
#include "LocalPath.hpp"
#include "LogFile.hpp"
#include "json/Serialize.hxx"
#include "util/StaticString.hxx"
#include "util/ConvertString.hpp"
#include "util/SpanCast.hxx"
#include "util/tstring.hpp"
#include "util/UTF8.hpp"
#include "Geo/AltitudeReference.hpp"
#include "thread/Mutex.hxx"
#include "co/Task.hxx"
#include "co/InvokeTask.hxx"
#include "lib/curl/Global.hxx"
#include "time/Convert.hxx"
#include "Language/Language.hpp"

#include <optional>
#include <algorithm>
#include <cstring>

#include <boost/json.hpp>

// Use full struct name to avoid collision with AirspaceClass::NOTAM enum
using NOTAMStruct = struct NOTAM;

#ifdef HAVE_HTTP

static constexpr std::time_t MIN_NOTAM_REFRESH_ATTEMPT_INTERVAL_SECONDS =
    120;

// Private implementation to avoid template issues  
struct NOTAMImpl {
  std::vector<NOTAMStruct> current_notams;
};

NOTAMGlue::NOTAMGlue(const NOTAMSettings &_settings, CurlGlobal &_curl)
  : RateLimiter(std::chrono::seconds(30), std::chrono::seconds(30)),
    settings(_settings), curl(_curl), 
    current_notams_impl(std::make_unique<NOTAMImpl>()),
    load_task(curl.GetEventLoop())
{
}

NOTAMGlue::~NOTAMGlue() = default;

void
NOTAMGlue::OnTimer(const GeoPoint &current_location)
{
  if (!settings.enabled) {
    LogDebug("NOTAM: Auto-refresh skipped - disabled in settings");
    return;
  }

  if (!current_location.IsValid()) {
    LogDebug("NOTAM: Auto-refresh skipped - invalid location");
    return;
  }

  // Check if manual-only mode (interval = 0)
  if (settings.refresh_interval_min == 0) {
    LogDebug("NOTAM: Auto-refresh skipped - manual-only mode");
    return;
  }

  // If we're already loading or a retry is pending, skip auto-refresh
  {
    const std::lock_guard<Mutex> lock(mutex);
    if (loading || retry_pending) {
      LogDebug("NOTAM: Auto-refresh skipped - loading={} retry_pending={}",
               loading, retry_pending);
      return;
    }
  }

  GeoPoint last_loc = GetLastUpdateLocation();
  std::time_t last_time = GetLastUpdateTime();
  std::time_t now = std::time(nullptr);
  
  // Check if time interval has elapsed since last successful fetch
  bool time_expired = (last_time == 0) || 
                      (now - last_time >= (std::time_t)(settings.refresh_interval_min * 60));
  
  // Also check if enough time has passed since last attempt (even if it failed)
  // This prevents rapid retries when there's no network connection
  bool enough_time_since_attempt =
      (now - last_attempt_time >= MIN_NOTAM_REFRESH_ATTEMPT_INTERVAL_SECONDS);
  
  // Check if location moved outside half the radius
  bool location_changed = last_loc.IsValid() && 
                          current_location.Distance(last_loc) > (settings.radius_km * 1000.0 / 2.0);
  
  if (!enough_time_since_attempt) {
    // Auto-refresh skipped - last attempt too recent
    return;
  }

  if (time_expired || location_changed) {
    UpdateLocation(current_location);
    return;
  }
}

void
NOTAMGlue::UpdateLocation(const GeoPoint &location)
{
  if (!location.IsValid()) {
    return;
  }

  // Check if settings are enabled
  if (!settings.enabled) {
    return;
  }

  // Check if already loading
  {
    const std::lock_guard<Mutex> lock(mutex);
    if (loading) {
      return; // Already loading, skip this request
    }
    loading = true;
    retry_pending = false;
    current_location = location;
    last_attempt_time = std::time(nullptr);  // Record attempt time
  }
  
  // Log only when we actually start a fetch
  LogFormat("NOTAM: Auto-refresh starting");
  
  // Cancel any pending retry since we're starting a new attempt
  Cancel();

  // Start async loading
  load_task.Start(LoadNOTAMsInternal(location), 
                  BIND_THIS_METHOD(OnLoadComplete));
}

void
NOTAMGlue::LoadNOTAMs(const GeoPoint &location, OperationEnvironment &operation)
{
  (void)operation; // Suppress unused parameter warning
  
  // Check if NOTAMs are enabled
  if (!settings.enabled) {
    return;
  }
  
  // Check if location is valid
  if (!location.IsValid()) {
    return;
  }
  
  // Check if already loading
  {
    const std::lock_guard lock(mutex);
    if (loading) {
      return;
    }
    loading = true;
    retry_pending = false;
    current_location = location;
  }
  
  // Cancel any pending retry since we're starting a new attempt
  Cancel();
  
  // Start async loading
  load_task.Start(LoadNOTAMsInternal(location), 
                  BIND_THIS_METHOD(OnLoadComplete));
}

std::vector<NOTAMStruct>
NOTAMGlue::GetNOTAMs(unsigned max_count) const
{
  const std::lock_guard<Mutex> lock(mutex);
  auto *impl = current_notams_impl.get();
  
  const auto &notams = impl->current_notams;
  const unsigned available_count = static_cast<unsigned>(notams.size());
  
  // If max_count is 0 or greater than available, return all NOTAMs
  const unsigned count = (max_count == 0 || max_count > available_count) 
                         ? available_count : max_count;
  
  LogDebug("NOTAM: GetNOTAMs max_count={}, available={}, returning={}",
           max_count, available_count, count);
  
  // Return a copy of the requested NOTAMs
  std::vector<NOTAMStruct> result;
  result.reserve(count);
  result.insert(result.end(), notams.begin(), notams.begin() + count);
  
  return result;
}

void
NOTAMGlue::Clear()
{
  const std::lock_guard<Mutex> lock(mutex);
  auto *impl = current_notams_impl.get();
  impl->current_notams.clear();
}

// Duplicate method removed

Co::InvokeTask
NOTAMGlue::LoadNOTAMsInternal(GeoPoint location)
{
  LogFormat("NOTAM: Starting LoadNOTAMsInternal for location %.6f,%.6f", 
            location.latitude.Degrees(), location.longitude.Degrees());

  try {
    // First, try to load from cache if not expired
    LogFormat("NOTAM: Checking cache expiration status");
    if (!IsCacheExpired()) {
      LogFormat("NOTAM: Cache is still valid, attempting to load from cache");
      std::vector<NOTAMStruct> cached_notams;
      if (LoadNOTAMsFromFile(cached_notams)) {
        const unsigned count = static_cast<unsigned>(cached_notams.size());
        LogFormat("NOTAM: Successfully loaded %u NOTAMs from cache", count);
        
        // Store cached results
        {
          const std::lock_guard<Mutex> lock(mutex);
          auto *impl = current_notams_impl.get();
          impl->current_notams = std::move(cached_notams);
        }
        LogFormat("NOTAM: Using cached data, fetch complete");
        co_return; // Use cached data, no need to fetch
      } else {
        LogFormat("NOTAM: Failed to load from cache file, will fetch fresh data");
      }
    } else {
      LogFormat("NOTAM: Cache is expired or doesn't exist, fetching fresh data");
    }
    
    // Cache is expired or doesn't exist, fetch fresh data from API
    LogFormat("NOTAM: Starting API fetch for radius %u km", settings.radius_km);
    
    // Use a simple progress listener (we could make this more sophisticated)
    class SimpleProgressListener : public ProgressListener {
    public:
      void SetProgressRange(unsigned range) noexcept override { (void)range; }
      void SetProgressPosition(unsigned position) noexcept override { (void)position; }
    };
    SimpleProgressListener progress;
    
    // Fetch raw GeoJSON using the client
    LogFormat("NOTAM: Calling FetchNOTAMsRaw...");
    auto raw_geojson = co_await NOTAMClient::FetchNOTAMsRaw(curl, settings, location, progress);
    LogFormat("NOTAM: Received GeoJSON response (%lu bytes)", (unsigned long)raw_geojson.size());
    
    // Save raw GeoJSON to file for caching
    LogFormat("NOTAM: Saving GeoJSON to cache file");
    SaveNOTAMsToFile(raw_geojson, location);
    LogFormat("NOTAM: Cache file saved successfully");
    
    // Parse GeoJSON to get NOTAMs for in-memory storage
    LogFormat("NOTAM: Parsing GeoJSON to extract NOTAMs");
    auto notams = NOTAMClient::ParseNOTAMGeoJSON(raw_geojson);
    const unsigned count = static_cast<unsigned>(notams.size());
    LogFormat("NOTAM: Parsed %u NOTAMs from GeoJSON", count);
    
    // Store the results
    {
      const std::lock_guard<Mutex> lock(mutex);
      auto *impl = current_notams_impl.get();
      impl->current_notams = std::move(notams);
    }
    
    LogFormat("NOTAM: Successfully completed fetch with %u NOTAMs", count);
    
    // Notify user of successful load
    StaticString<100> msg;
    msg.Format(_("Loaded %u NOTAMs"), count);
    Message::AddMessage(msg.c_str());
    
  } catch (const std::exception &e) {
    LogFormat("NOTAM: Error during fetch: %s", e.what());
    
    // Keep existing NOTAMs on fetch failure - stale data is better than no data
    LogFormat("NOTAM: Keeping existing NOTAMs after fetch error");
    
    // Notify user of error
    Message::AddMessage(_("Failed to load NOTAMs"));
  }
}

void
NOTAMGlue::OnLoadComplete(std::exception_ptr error) noexcept
{
  // Reset loading flag
  bool schedule_retry = false;
  {
    const std::lock_guard<Mutex> lock(mutex);
    loading = false;
    if (error) {
      retry_pending = true;
      schedule_retry = true;
    } else {
      retry_pending = false;
    }
  }
  
  if (schedule_retry) {
    // Failed - schedule retry with fixed 30-second delay
    LogFormat("NOTAM: Fetch failed, scheduling retry in 30 seconds");
    Trigger();
  } else {
    // Success - cancel any pending retry
    Cancel();
    
    // Update the airspace database
    if (data_components && data_components->airspaces) {
      try {
        LogFormat("NOTAM: Updating airspace database with loaded NOTAMs");
        UpdateAirspaces(*data_components->airspaces);
        LogFormat("NOTAM: Airspace database updated successfully");
      } catch (const std::exception &e) {
        LogFormat("NOTAM: Error updating airspace database: %s", e.what());
      }
    }
    
    // Notify listeners that NOTAMs have been updated
    NotifyListeners();
  }
}

void
NOTAMGlue::AddListener(NOTAMListener &listener) noexcept
{
  const std::lock_guard<Mutex> lock(mutex);
  listeners.push_back(&listener);
}

void
NOTAMGlue::RemoveListener(NOTAMListener &listener) noexcept
{
  const std::lock_guard<Mutex> lock(mutex);
  auto it = std::find(listeners.begin(), listeners.end(), &listener);
  if (it != listeners.end()) {
    listeners.erase(it);
  }
}

void
NOTAMGlue::NotifyListeners() noexcept
{
  std::vector<NOTAMListener *> listeners_copy;
  {
    const std::lock_guard<Mutex> lock(mutex);
    listeners_copy = listeners;
  }
  
  for (auto *listener : listeners_copy) {
    listener->OnNOTAMsUpdated();
  }
}

void
NOTAMGlue::Run()
{
  LogFormat("NOTAM: Retry timer fired, attempting fetch again");
  
  GeoPoint location;
  {
    const std::lock_guard<Mutex> lock(mutex);
    retry_pending = false;
    if (loading || !current_location.IsValid()) {
      return;
    }
    location = current_location;
  }
  
  // Trigger a new fetch attempt (outside the lock)
  UpdateLocation(location);
}

unsigned
NOTAMGlue::LoadCachedNOTAMs()
{
  std::vector<NOTAMStruct> cached_notams;
  
  // Log the cache file path for debugging
  auto file_path = GetNOTAMCacheFilePath();
  LogFormat("NOTAM: Attempting to load cache from: %s", WideToUTF8Converter(file_path.c_str()).c_str());
  
  // Try to load from cache file
  if (LoadNOTAMsFromFile(cached_notams)) {
    const unsigned count = static_cast<unsigned>(cached_notams.size());
    
    // Store cached results in memory
    {
      const std::lock_guard<Mutex> lock(mutex);
      auto *impl = current_notams_impl.get();
      impl->current_notams = std::move(cached_notams);
    }
        
    LogFormat("NOTAM: Loaded %u NOTAMs from cache", count);
    return count;
  }
  
  LogFormat("NOTAM: No cached NOTAMs found at: %s", WideToUTF8Converter(file_path.c_str()).c_str());
  return 0;
}

bool
NOTAMGlue::LoadCachedNOTAMsAndUpdate(Airspaces &airspaces)
{
  if (!settings.enabled) {
    LogDebug("NOTAM: Startup cache load skipped - disabled in settings");
    return false;
  }

  std::vector<NOTAMStruct> cached_notams;
  if (!LoadNOTAMsFromFile(cached_notams)) {
    LogFormat("NOTAM: No cached NOTAMs to apply");
    return false;
  }

  const unsigned count = static_cast<unsigned>(cached_notams.size());
  {
    const std::lock_guard<Mutex> lock(mutex);
    auto *impl = current_notams_impl.get();
    impl->current_notams = std::move(cached_notams);
  }

  LogFormat("NOTAM: Loaded %u NOTAMs from cache for startup", count);

  UpdateAirspaces(airspaces);
  NotifyListeners();

  return true;
}

void
NOTAMGlue::InvalidateCache()
{
  auto file_path = GetNOTAMCacheFilePath();
  
  if (File::Exists(file_path)) {
    LogFormat("NOTAM: Invalidating cache file: %s", WideToUTF8Converter(file_path.c_str()).c_str());
    File::Delete(file_path);
    LogFormat("NOTAM: Cache invalidated successfully");
  } else {
    LogFormat("NOTAM: No cache file to invalidate at: %s", WideToUTF8Converter(file_path.c_str()).c_str());
  }
}

// Forward declarations
static bool ShouldDisplayNOTAM(const NOTAMStruct &notam, const NOTAMSettings &settings);
static bool IsQCodeHidden(const std::string &qcode, const char *hidden_list);

unsigned
NOTAMGlue::GetTotalCount() const
{
  const std::lock_guard<Mutex> lock(mutex);
  auto *impl = current_notams_impl.get();
  return static_cast<unsigned>(impl->current_notams.size());
}

unsigned
NOTAMGlue::GetFilteredCount() const
{
  const std::lock_guard<Mutex> lock(mutex);
  auto *impl = current_notams_impl.get();
  
  unsigned count = 0;
  for (const auto &notam : impl->current_notams) {
    if (ShouldDisplayNOTAM(notam, settings)) {
      count++;
    }
  }
  return count;
}

NOTAMGlue::FilterStats
NOTAMGlue::GetFilterStats() const
{
  const std::lock_guard<Mutex> lock(mutex);
  auto *impl = current_notams_impl.get();
  
  FilterStats stats;
  stats.total = static_cast<unsigned>(impl->current_notams.size());
  stats.filtered_by_ifr = 0;
  stats.filtered_by_time = 0;
  stats.filtered_by_qcode = 0;
  stats.filtered_by_radius = 0;
  stats.final_count = 0;
  
  for (const auto &notam : impl->current_notams) {
    // Count how many fail IFR filter (independently)
    if (!settings.show_ifr && !notam.traffic.empty() && notam.traffic == "I") {
      stats.filtered_by_ifr++;
    }
    
    // Count how many fail time filter (independently)
    if (settings.show_only_effective) {
      auto now = std::chrono::system_clock::now();
      if (now < notam.start_time || now > notam.end_time) {
        stats.filtered_by_time++;
      }
    }
    
    // Count how many fail Q-code filter (independently)
    const auto &qcode = notam.feature_type;
    if (!qcode.empty() && IsQCodeHidden(qcode, WideToUTF8Converter(settings.hidden_qcodes.c_str()).c_str())) {
      stats.filtered_by_qcode++;
    }
    
    // Count how many fail radius filter (independently)
    if (settings.max_radius_m > 0) {
      if (notam.geometry.radius_meters > settings.max_radius_m) {
        stats.filtered_by_radius++;
      }
    }
    
    // Use the main filter function for final count
    if (ShouldDisplayNOTAM(notam, settings)) {
      stats.final_count++;
    }
  }
  
  return stats;
}

std::optional<struct NOTAM>
NOTAMGlue::FindNOTAMByNumber(const std::string &number) const
{
  const std::lock_guard<Mutex> lock(mutex);
  auto *impl = current_notams_impl.get();
  
  for (const auto &notam : impl->current_notams) {
    if (notam.number == number) {
      return notam; // Return a copy while holding the lock
    }
  }
  
  return std::nullopt;
}

/**
 * Check if a Q-code matches any prefix in the hidden list
 * E.g., if hidden_list contains "QO", it will match "QOLX", "QOAX", etc.
 * Supports both space and comma separators.
 */
static bool
IsQCodeHidden(const std::string &qcode, const char *hidden_list)
{
  if (qcode.empty() || !hidden_list || *hidden_list == '\0')
    return false;
  
  const size_t list_len = strlen(hidden_list);
  
  // Parse space or comma-separated list and check if qcode starts with any prefix
  size_t start = 0;
  while (start < list_len) {
    // Skip leading whitespace
    while (start < list_len && 
           (hidden_list[start] == ' ' || hidden_list[start] == '\t'))
      start++;
    
    if (start >= list_len)
      break;
    
    // Find next separator (space or comma) or end of string
    size_t end = start;
    while (end < list_len && 
           hidden_list[end] != ' ' && 
           hidden_list[end] != '\t' &&
           hidden_list[end] != ',')
      end++;
    
    // Check if qcode starts with this prefix
    size_t prefix_len = end - start;
    if (prefix_len > 0 && qcode.compare(0, prefix_len, hidden_list + start, prefix_len) == 0) {
      return true;
    }
    
    start = end + 1;
  }
  
  return false;
}

/**
 * Check if a NOTAM should be displayed based on filter settings
 */
static bool
ShouldDisplayNOTAM(const NOTAMStruct &notam, const NOTAMSettings &settings)
{
  // Check IFR filter (I=IFR-only, V=VFR-only, IV=both)
  // When show_ifr is false, only filter out IFR-only NOTAMs (I)
  // IV NOTAMs apply to both IFR and VFR, so they should always be shown
  if (!settings.show_ifr && !notam.traffic.empty()) {
    if (notam.traffic == "I") {
      LogDebug("NOTAM Filter: {} is IFR-only traffic ({}), filtered out",
               notam.number.c_str(), notam.traffic.c_str());
      return false;
    }
  }
  
  // Check if currently effective (if filter enabled)
  if (settings.show_only_effective) {
    auto now = std::chrono::system_clock::now();
    if (now < notam.start_time || now > notam.end_time) {
      LogDebug("NOTAM Filter: {} not currently effective, filtered out", notam.number.c_str());
      return false;
    }
  }
  
  // Check radius filter
  if (settings.max_radius_m > 0) {
    if (notam.geometry.radius_meters > settings.max_radius_m) {
      LogDebug("NOTAM Filter: {} radius {:.0f} m exceeds limit {} m, filtered out",
               notam.number.c_str(), notam.geometry.radius_meters, settings.max_radius_m);
      return false;
    }
  }
  
  // Check Q-code filters
  const auto &qcode = notam.feature_type;
  
  if (qcode.empty()) {
    // No Q-code - show by default
    return true;
  }
  
  LogDebug("NOTAM Filter: {} Q-code='{}'", notam.number.c_str(), qcode.c_str());
  
  // Check if this Q-code matches any prefix in the hidden list
  if (IsQCodeHidden(qcode, WideToUTF8Converter(settings.hidden_qcodes.c_str()).c_str())) {
    LogDebug("NOTAM Filter: {} Q-code {} is hidden", notam.number.c_str(), qcode.c_str());
    return false;
  }
  
  // Not filtered - show it
  return true;
}

void
NOTAMGlue::UpdateAirspaces(Airspaces &airspaces)
{
  const std::lock_guard<Mutex> lock(mutex);
  auto *impl = current_notams_impl.get();
  
  LogFormat("NOTAM: UpdateAirspaces - converting %u NOTAMs to airspaces", 
            static_cast<unsigned>(impl->current_notams.size()));

  // Save all non-NOTAM airspaces
  // ToDo: Optimize by removing only existing NOTAM airspaces
  std::vector<AirspacePtr> saved_airspaces;
  for (const auto &airspace : airspaces.QueryAll()) {
    const auto &current = airspace.GetAirspace();
    if (current.GetClassOrType() != AirspaceClass::NOTAM) {
      saved_airspaces.push_back(airspace.GetAirspacePtr());
    }
  }
  
  LogFormat("NOTAM: Saved %u non-NOTAM airspaces", 
            static_cast<unsigned>(saved_airspaces.size()));
  
  // Clear all airspaces
  airspaces.Clear();
  
  // Restore non-NOTAM airspaces
  for (const auto &airspace : saved_airspaces) {
    airspaces.Add(airspace);
  }
  
  LogFormat("NOTAM: Restored %u non-NOTAM airspaces", 
            static_cast<unsigned>(saved_airspaces.size()));
  
  // Add filtered NOTAM airspaces
  unsigned added_count = 0;
  unsigned filtered_count = 0;
  
  for (const auto &notam : impl->current_notams) {
    try {
      // Apply filtering
      if (!ShouldDisplayNOTAM(notam, settings)) {
        filtered_count++;
        continue;
      }
      
      // Create airspace based on NOTAM geometry
      AirspacePtr airspace_ptr;
      
      switch (notam.geometry.type) {
        case NOTAM::NOTAMGeometry::Type::CIRCLE: {
          auto circle = std::make_shared<AirspaceCircle>(
            GeoPoint{Angle::Degrees(notam.geometry.center.longitude), 
                     Angle::Degrees(notam.geometry.center.latitude)},
            notam.geometry.radius_meters);
          airspace_ptr = circle;
          break;
        }
        
        case NOTAM::NOTAMGeometry::Type::POLYGON: {
          // Build vector of GeoPoints for the polygon
          std::vector<GeoPoint> polygon_points;
          for (const auto &point : notam.geometry.polygon_points) {
            polygon_points.emplace_back(Angle::Degrees(point.longitude), 
                                        Angle::Degrees(point.latitude));
          }
          
          if (!polygon_points.empty()) {
            auto polygon = std::make_shared<AirspacePolygon>(polygon_points);
            airspace_ptr = polygon;
          }
          break;
        }
        
        case NOTAM::NOTAMGeometry::Type::POINT:
        default: {
          // For points, create a small circle (1000m radius)
          auto circle = std::make_shared<AirspaceCircle>(
            GeoPoint{Angle::Degrees(notam.geometry.center.longitude), 
                     Angle::Degrees(notam.geometry.center.latitude)},
            1000.0); // 1km radius
          airspace_ptr = circle;
          break;
        }
      }
      
      if (airspace_ptr) {
        // Create the airspace main name, truncate if too long
        std::string main_text = notam.text;
        if (main_text.empty()) {
          main_text = "NOTAM";
        }
        if (main_text.length() > 150) {
          main_text = main_text.substr(0, 147) + "...";
        }
        
        // Convert UTF-8 strings to TCHAR (wchar_t on Windows, char on Unix)
        UTF8ToWideConverter main_text_conv(main_text.c_str());
        UTF8ToWideConverter number_conv(notam.number.c_str());
        
        // Use converted strings, with fallback to empty if conversion failed
        tstring notam_name = main_text_conv.IsValid() ? tstring(main_text_conv.c_str()) : _T("NOTAM");
        tstring notam_station = number_conv.IsValid() ? tstring(number_conv.c_str()) : _T("");
        
        // Use the parsed AirspaceAltitude objects directly
        AirspaceAltitude base = notam.lower_altitude;
        AirspaceAltitude top = notam.upper_altitude;
        
        // Set reasonable defaults if altitudes are invalid
        // Check the reference field for garbage values (valid: 0=AGL, 1=MSL, 2=STD)
        if (base.reference != AltitudeReference::AGL && 
            base.reference != AltitudeReference::MSL && 
            base.reference != AltitudeReference::STD) {
          // Uninitialized or invalid - set to ground level
          base.reference = AltitudeReference::AGL;
          base.altitude_above_terrain = 0;
          base.altitude = 0;
          base.flight_level = 0;
        }
        
        if (top.reference != AltitudeReference::AGL && 
            top.reference != AltitudeReference::MSL && 
            top.reference != AltitudeReference::STD) {
          // Uninitialized or invalid - set to high MSL altitude
          top.reference = AltitudeReference::MSL;
          top.altitude = 9999;
          top.altitude_above_terrain = 0;
          top.flight_level = 0;
        }
        
        // Set airspace properties using SetProperties method
        airspace_ptr->SetProperties(
          std::move(notam_name),        // name: NOTAM detailed text
          std::move(notam_station),     // station_name: short identifier  
          TransponderCode::Null(), // transponder_code
          AirspaceClass::NOTAM, // class
          AirspaceClass::NOTAM, // type
          base,
          top
        );
        
        // Add to airspace database
        airspaces.Add(airspace_ptr);
        added_count++;
        
        LogDebug("NOTAM: Added airspace for NOTAM '{}' (type={}, radius={:.0f}m)",
                 notam.number.c_str(), static_cast<int>(notam.geometry.type),
                 notam.geometry.radius_meters);
      }
    } catch (const std::exception &e) {
      LogFormat("NOTAM: Error creating airspace for NOTAM '%s': %s", 
                notam.number.c_str(), e.what());
    }
  }
  
  // Optimize the airspace tree after adding NOTAMs
  airspaces.Optimise();
  
  LogFormat("NOTAM: UpdateAirspaces completed - added %u NOTAMs, filtered %u NOTAMs",
            added_count, filtered_count);
}

AllocatedPath
NOTAMGlue::GetNOTAMCacheFilePath() const
{
  // Use fixed filename in main XCSoarData directory
  // We only have one cache file to KISS, no merging, no modifications
  return LocalPath(_T("notams.json"));
}

void
NOTAMGlue::SaveNOTAMsToFile(const std::string &geojson_response, const GeoPoint &location) const
{
  try {
    auto file_path = GetNOTAMCacheFilePath();
    LogFormat("NOTAM: Saving GeoJSON to cache: %s", WideToUTF8Converter(file_path.c_str()).c_str());
    
    // Validate UTF-8 before saving - boost::json will validate during parse anyway,
    // but we check here to avoid saving potentially corrupted data
    if (!ValidateUTF8(geojson_response)) {
      LogFormat("NOTAM: Warning - GeoJSON contains invalid UTF-8, skipping cache save");
      return;
    }
    
    boost::system::error_code ec;
    boost::json::value geojson = boost::json::parse(geojson_response, ec);
    if (ec) {
      LogFormat("NOTAM: Error parsing GeoJSON: %s", ec.message().c_str());
      return;
    }

    boost::json::object wrapper;
    wrapper["xcsoar_timestamp"] = static_cast<std::int64_t>(std::time(nullptr));
    wrapper["xcsoar_location_lat"] = location.latitude.Degrees();
    wrapper["xcsoar_location_lon"] = location.longitude.Degrees();
    wrapper["xcsoar_radius_km"] = settings.radius_km;
    wrapper["xcsoar_refresh_interval_min"] = settings.refresh_interval_min;
    wrapper["geojson"] = std::move(geojson);

    StringOutputStream sos;
    Json::Serialize(sos, boost::json::value(std::move(wrapper)));
    std::string json_with_metadata = std::move(sos).GetValue();
    
    // Write to file
    FileOutputStream file(Path(file_path.c_str()));
    file.Write(std::as_bytes(std::span{json_with_metadata}));
    file.Commit();
    
    LogFormat("NOTAM: Saved %u bytes of GeoJSON to cache", static_cast<unsigned>(json_with_metadata.size()));
  } catch (const std::exception &e) {
    LogFormat("NOTAM: Error saving GeoJSON to file: %s", e.what());
  }
}

bool
NOTAMGlue::LoadNOTAMsFromFile(std::vector<NOTAMStruct> &notams) const
{
  try {
    auto file_path = GetNOTAMCacheFilePath();
    LogFormat("NOTAM: LoadNOTAMsFromFile attempting to load: %s", WideToUTF8Converter(file_path.c_str()).c_str());
    
    std::string json_content;
    try {
      FileReader file(Path(file_path.c_str()));
      
      // Read the entire file to get the JSON data
      char buffer[4096];
      while (true) {
        size_t bytes_read = file.Read(std::as_writable_bytes(std::span{buffer}));
        if (bytes_read == 0) break;
        json_content.append(buffer, bytes_read);
      }
    } catch (const std::exception &e) {
      LogFormat("NOTAM: LoadNOTAMsFromFile failed to open file: %s - %s", WideToUTF8Converter(file_path.c_str()).c_str(), e.what());
      return false; // File doesn't exist or can't be opened
    }
    
    LogFormat("NOTAM: LoadNOTAMsFromFile read %u bytes from file", static_cast<unsigned>(json_content.size()));
    
    if (json_content.empty()) {
      LogFormat("NOTAM: LoadNOTAMsFromFile file is empty");
      return false;
    }
    
    // Log first 200 characters of JSON content for debugging
    std::string preview = json_content.length() > 200 ? json_content.substr(0, 200) + "..." : json_content;
    LogFormat("NOTAM: LoadNOTAMsFromFile JSON content preview: %s", preview.c_str());
    
    // Extract the GeoJSON from the wrapper object
    std::string geojson_content;
    
    // Look for the "geojson" field in the wrapper
    size_t geojson_pos = json_content.find("\"geojson\":");
    if (geojson_pos != std::string::npos) {
      // Find the start of the GeoJSON value (after the colon)
      size_t value_start = json_content.find(':', geojson_pos) + 1;
      
      // Find the end of the GeoJSON object by counting braces
      // Assumes the GeoJSON is a well-formed JSON object @TODO
      size_t brace_count = 0;
      size_t value_end = value_start;
      bool in_string = false;
      bool escaped = false;
      
      for (size_t i = value_start; i < json_content.length(); ++i) {
        char c = json_content[i];
        
        if (escaped) {
          escaped = false;
          continue;
        }
        
        if (c == '\\') {
          escaped = true;
          continue;
        }
        
        if (c == '"') {
          in_string = !in_string;
          continue;
        }
        
        if (!in_string) {
          if (c == '{') {
            brace_count++;
          } else if (c == '}') {
            brace_count--;
            if (brace_count == 0) {
              value_end = i + 1;
              break;
            }
          }
        }
      }
      
      if (brace_count == 0 && value_end > value_start) {
        geojson_content = json_content.substr(value_start, value_end - value_start);
      } else {
        LogFormat("NOTAM: LoadNOTAMsFromFile failed to extract GeoJSON from wrapper");
        return false;
      }
    } else {
      // No wrapper found, try parsing the content directly as GeoJSON as a fallback
      geojson_content = json_content;
    }
    
    // Parse the GeoJSON content using the Client's parser
    notams.clear();
    
    LogFormat("NOTAM: LoadNOTAMsFromFile parsing GeoJSON...");
    
    try {
      // Use the Client's GeoJSON parser
      notams = NOTAMClient::ParseNOTAMGeoJSON(geojson_content);
      LogFormat("NOTAM: LoadNOTAMsFromFile parsed %u NOTAMs from GeoJSON", static_cast<unsigned>(notams.size()));
      return true;
    } catch (const std::exception &e) {
      LogFormat("NOTAM: LoadNOTAMsFromFile GeoJSON parse error: %s", e.what());
      return false;
    }
  } catch (const std::exception &e) {
    LogFormat("NOTAM: LoadNOTAMsFromFile exception: %s", e.what());
    return false;
  }
}

std::time_t
NOTAMGlue::GetLastUpdateTime() const
{
  try {
    auto file_path = GetNOTAMCacheFilePath();
    
    FileReader file(Path(file_path.c_str()));
    
    // Read the entire file to get the JSON data
    std::string json_content;
    char buffer[4096];
    while (true) {
      size_t bytes_read = file.Read(std::as_writable_bytes(std::span{buffer}));
      if (bytes_read == 0) break;
      json_content.append(buffer, bytes_read);
    }
    
    // Look for XCSoar timestamp in the content
    size_t timestamp_pos = json_content.find("\"xcsoar_timestamp\":");
    if (timestamp_pos != std::string::npos) {
      // Extract timestamp value
      size_t value_start = json_content.find(':', timestamp_pos) + 1;
      size_t value_end = json_content.find(',', value_start);
      if (value_end == std::string::npos) {
        value_end = json_content.find('}', value_start);
      }
      
      if (value_start != std::string::npos && value_end != std::string::npos) {
        std::string timestamp_str = json_content.substr(value_start, value_end - value_start);
        // Remove whitespace
        timestamp_str.erase(0, timestamp_str.find_first_not_of(" \t"));
        timestamp_str.erase(timestamp_str.find_last_not_of(" \t") + 1);
        
        try {
          return std::stoll(timestamp_str);
        } catch (const std::exception &) {
          return 0; // Invalid timestamp
        }
      }
    }
    
    return 0; // No timestamp found
  } catch (const std::exception &) {
    return 0; // Error reading file
  }
}

GeoPoint
NOTAMGlue::GetLastUpdateLocation() const
{
  try {
    auto file_path = GetNOTAMCacheFilePath();
    FileReader file(Path(file_path.c_str()));

    std::string json_content;
    char buffer[4096];
    while (true) {
      size_t bytes_read = file.Read(std::as_writable_bytes(std::span{buffer}));
      if (bytes_read == 0) break;
      json_content.append(buffer, bytes_read);
    }

    // Find latitude and longitude metadata stored during SaveNOTAMsToFile()
    size_t lat_pos = json_content.find("\"xcsoar_location_lat\":");
    size_t lon_pos = json_content.find("\"xcsoar_location_lon\":");
    if (lat_pos == std::string::npos || lon_pos == std::string::npos)
      return GeoPoint::Invalid();

    // Extract latitude
    size_t lat_value_start = json_content.find(':', lat_pos) + 1;
    size_t lat_value_end = json_content.find(',', lat_value_start);
    if (lat_value_end == std::string::npos)
      lat_value_end = json_content.find('}', lat_value_start);
    if (lat_value_start == std::string::npos || lat_value_end == std::string::npos)
      return GeoPoint::Invalid();
    std::string lat_str = json_content.substr(lat_value_start, lat_value_end - lat_value_start);
    lat_str.erase(0, lat_str.find_first_not_of(" \t"));
    lat_str.erase(lat_str.find_last_not_of(" \t") + 1);

    // Extract longitude
    size_t lon_value_start = json_content.find(':', lon_pos) + 1;
    size_t lon_value_end = json_content.find(',', lon_value_start);
    if (lon_value_end == std::string::npos)
      lon_value_end = json_content.find('}', lon_value_start);
    if (lon_value_start == std::string::npos || lon_value_end == std::string::npos)
      return GeoPoint::Invalid();
    std::string lon_str = json_content.substr(lon_value_start, lon_value_end - lon_value_start);
    lon_str.erase(0, lon_str.find_first_not_of(" \t"));
    lon_str.erase(lon_str.find_last_not_of(" \t") + 1);

    try {
      double lat = std::stod(lat_str);
      double lon = std::stod(lon_str);
      return GeoPoint(Angle::Degrees(lon), Angle::Degrees(lat));
    } catch (const std::exception &) {
      return GeoPoint::Invalid();
    }
  } catch (const std::exception &) {
    return GeoPoint::Invalid();
  }
}

bool
NOTAMGlue::IsCacheExpired() const
{
  try {
    auto file_path = GetNOTAMCacheFilePath();
    
    try {
      FileReader file(Path(file_path.c_str()));
      
      // Read the entire file to get the JSON data
      std::string json_content;
      char buffer[4096];
      while (true) {
        size_t bytes_read = file.Read(std::as_writable_bytes(std::span{buffer}));
        if (bytes_read == 0) break;
        json_content.append(buffer, bytes_read);
      }
      
      // Extract metadata from cache file
      std::time_t cached_time = 0;
      double cached_lat = 0.0;
      double cached_lon = 0.0;
      
      // Look for XCSoar timestamp
      size_t timestamp_pos = json_content.find("\"xcsoar_timestamp\":");
      if (timestamp_pos != std::string::npos) {
        size_t value_start = json_content.find(':', timestamp_pos) + 1;
        size_t value_end = json_content.find(',', value_start);
        if (value_end != std::string::npos) {
          std::string timestamp_str = json_content.substr(value_start, value_end - value_start);
          timestamp_str.erase(0, timestamp_str.find_first_not_of(" \t"));
          timestamp_str.erase(timestamp_str.find_last_not_of(" \t") + 1);
          try {
            cached_time = std::stoll(timestamp_str);
          } catch (const std::exception &) {
            return true; // Invalid timestamp means expired
          }
        }
      } else {
        return true; // No timestamp found means expired
      }
      
      // Look for cached location latitude
      size_t lat_pos = json_content.find("\"xcsoar_location_lat\":");
      if (lat_pos != std::string::npos) {
        size_t value_start = json_content.find(':', lat_pos) + 1;
        size_t value_end = json_content.find(',', value_start);
        if (value_end != std::string::npos) {
          std::string lat_str = json_content.substr(value_start, value_end - value_start);
          lat_str.erase(0, lat_str.find_first_not_of(" \t"));
          lat_str.erase(lat_str.find_last_not_of(" \t") + 1);
          try {
            cached_lat = std::stod(lat_str);
          } catch (const std::exception &) {
            return true; // Invalid location means expired
          }
        }
      }
      
      // Look for cached location longitude
      size_t lon_pos = json_content.find("\"xcsoar_location_lon\":");
      if (lon_pos != std::string::npos) {
        size_t value_start = json_content.find(':', lon_pos) + 1;
        size_t value_end = json_content.find(',', value_start);
        if (value_end != std::string::npos) {
          std::string lon_str = json_content.substr(value_start, value_end - value_start);
          lon_str.erase(0, lon_str.find_first_not_of(" \t"));
          lon_str.erase(lon_str.find_last_not_of(" \t") + 1);
          try {
            cached_lon = std::stod(lon_str);
          } catch (const std::exception &) {
            return true; // Invalid location means expired
          }
        }
      }
      
      // Check if cache is expired by time
      std::time_t current_time = std::time(nullptr);
      std::time_t max_age_seconds = settings.refresh_interval_min * 60;
      
      if ((current_time - cached_time) > max_age_seconds) {
        return true; // Expired by time
      }
      
      // Check if current location is too far from cached location
      // Copy current_location while holding mutex to avoid data race
      GeoPoint location_copy;
      {
        const std::lock_guard<Mutex> lock(mutex);
        location_copy = current_location;
      }
      
      if (location_copy.IsValid()) {
        GeoPoint cached_location{Angle::Degrees(cached_lon), Angle::Degrees(cached_lat)};
        double distance_m = location_copy.Distance(cached_location);
        double radius_m = settings.radius_km * 1000.0;
        
        // Cache is expired if we've moved more than the radius away
        if (distance_m > radius_m) {
          return true; // Expired by location
        }
      }
      
      return false; // Cache is still valid
    } catch (const std::exception &e) {
      return true; // Error means treat as expired
    }
  } catch (const std::exception &e) {
    return true; // Error means treat as expired
  }
}

#endif // HAVE_HTTP
