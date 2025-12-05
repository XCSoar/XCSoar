// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The XCSoar Project

#include "Glue.hpp"
#include "NOTAM.hpp"
#include "Client.hpp"
#include "Components.hpp"
#include "DataComponents.hpp"
#include "Message.hpp"
#include "util/StaticString.hxx"

// Use full struct name to avoid collision with AirspaceClass::NOTAM enum
using NOTAMStruct = struct NOTAM;
#include "thread/Mutex.hxx"
#include "lib/curl/Global.hxx"
#include "Operation/Operation.hpp"
#include "Operation/ProgressListener.hpp"
#include "co/Task.hxx"
#include "co/InvokeTask.hxx"
#include "system/Path.hpp"
#include "LocalPath.hpp"
#include "io/FileOutputStream.hxx"
#include "io/FileReader.hxx"
#include "system/FileUtil.hpp"
#include "util/SpanCast.hxx"
#include "time/Convert.hxx"
#include "LogFile.hpp"
#include <boost/json.hpp>

#include "system/Path.hpp"
#include <cstring>

// Global components access for airspace database
#include "Components.hpp"
#include "DataComponents.hpp"

// Airspace system includes
#include "Airspace/Airspaces.hpp"
#include "Airspace/AirspaceCircle.hpp"
#include "Airspace/AirspacePolygon.hpp"
#include "Airspace/AirspaceClass.hpp"
#include "Airspace/AirspaceAltitude.hpp"
#include "TransponderCode.hpp"
#include "util/tstring.hpp"
#include "Geo/AltitudeReference.hpp"

#ifdef HAVE_HTTP

// Private implementation to avoid template issues  
struct NOTAMImpl {
  std::vector<NOTAMStruct> current_notams;
};

NOTAMGlue::NOTAMGlue(const NOTAMSettings &_settings, CurlGlobal &_curl)
  : settings(_settings), curl(_curl), 
    current_notams_impl(new NOTAMImpl()),
    load_task(curl.GetEventLoop())
{
}

NOTAMGlue::~NOTAMGlue() {
  delete static_cast<NOTAMImpl*>(current_notams_impl);
}

void
NOTAMGlue::OnTimer(const GeoPoint &current_location)
{
  if (!settings.enabled || !current_location.IsValid())
    return;

  // Check if manual-only mode (interval = 0)
  if (settings.refresh_interval_min == 0)
    return;

  GeoPoint last_loc = GetLastUpdateLocation();
  std::time_t last_time = GetLastUpdateTime();
  std::time_t now = std::time(nullptr);
  
  // Check if time interval has elapsed
  bool time_expired = (last_time == 0) || 
                      (now - last_time >= settings.refresh_interval_min * 60);
  
  // Check if location moved outside half the radius
  bool location_changed = last_loc.IsValid() && 
                          current_location.Distance(last_loc) > (settings.radius_km * 1000.0 / 2.0);
  
  if (time_expired || location_changed) {
    LogFormat("NOTAM: Auto-refresh triggered (time_expired=%d, location_changed=%d)",
              (int)time_expired, (int)location_changed);
    UpdateLocation(current_location);
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
    current_location = location;
  }

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
    current_location = location;
  }
  
  // Start async loading
  load_task.Start(LoadNOTAMsInternal(location), 
                  BIND_THIS_METHOD(OnLoadComplete));
}

unsigned
NOTAMGlue::GetNOTAMs(void* buffer, unsigned max_count) const
{
  const std::lock_guard<Mutex> lock(mutex);
  auto *impl = static_cast<const NOTAMImpl*>(current_notams_impl);
  
  const auto &notams = impl->current_notams;
  const unsigned available_count = static_cast<unsigned>(notams.size());
  
  // If max_count is 0, return total available count
  if (max_count == 0) {
    LogFormat("NOTAM: GetNOTAMs called with max_count=0, returning available=%u", available_count);
    return available_count;
  }
  
  const unsigned count = std::min(max_count, available_count);
  
  LogFormat("NOTAM: GetNOTAMs called with max_count=%u, available=%u, returning=%u", 
            max_count, available_count, count);
  
  if (buffer && count > 0) {
    // Properly copy NOTAM objects using copy constructor
    NOTAMStruct* notam_buffer = static_cast<NOTAMStruct*>(buffer);
    for (unsigned i = 0; i < count; ++i) {
      notam_buffer[i] = notams[i];
    }
  }
  
  return count;
}

void
NOTAMGlue::Clear()
{
  const std::lock_guard<Mutex> lock(mutex);
  auto *impl = static_cast<NOTAMImpl*>(current_notams_impl);
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
          auto *impl = static_cast<NOTAMImpl*>(current_notams_impl);
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
    LogFormat("NOTAM: Starting API fetch for radius %.1u km", settings.radius_km);
    
    // Use a simple progress listener (we could make this more sophisticated)
    class SimpleProgressListener : public ProgressListener {
    public:
      void SetProgressRange(unsigned range) noexcept override { (void)range; }
      void SetProgressPosition(unsigned position) noexcept override { (void)position; }
    } progress;
    
    // Fetch raw GeoJSON using the client
    LogFormat("NOTAM: Calling FetchNOTAMsRaw...");
    auto raw_geojson = co_await NOTAMClient::FetchNOTAMsRaw(curl, settings, location, progress);
    LogFormat("NOTAM: Received GeoJSON response (%zu bytes)", raw_geojson.size());
    
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
      auto *impl = static_cast<NOTAMImpl*>(current_notams_impl);
      impl->current_notams = std::move(notams);
    }
    
    LogFormat("NOTAM: Successfully completed fetch with %u NOTAMs", count);
    
    // Notify user of successful load
    StaticString<100> msg;
    msg.Format(_T("Loaded %u NOTAMs"), count);
    Message::AddMessage(msg.c_str());
    
  } catch (const std::exception &e) {
    LogFormat("NOTAM: Error during fetch: %s", e.what());
    
    // Log error and clear any existing NOTAMs
    {
      const std::lock_guard<Mutex> lock(mutex);
      auto *impl = static_cast<NOTAMImpl*>(current_notams_impl);
      impl->current_notams.clear();
    }
    LogFormat("NOTAM: Cleared NOTAMs due to error");
    
    // Notify user of error
    Message::AddMessage(_T("Failed to load NOTAMs"));
  }
}

void
NOTAMGlue::OnLoadComplete(std::exception_ptr error) noexcept
{
  // Reset loading flag
  {
    const std::lock_guard<Mutex> lock(mutex);
    loading = false;
  }
  
  // If load was successful, update the airspace database
  if (!error && data_components && data_components->airspaces) {
    try {
      LogFormat("NOTAM: Updating airspace database with loaded NOTAMs");
      UpdateAirspaces(*data_components->airspaces);
      LogFormat("NOTAM: Airspace database updated successfully");
    } catch (const std::exception &e) {
      LogFormat("NOTAM: Error updating airspace database: %s", e.what());
    }
  }
}

unsigned
NOTAMGlue::LoadCachedNOTAMs()
{
  std::vector<NOTAMStruct> cached_notams;
  
  // Log the cache file path for debugging
  auto file_path = GetNOTAMCacheFilePath();
  LogFormat("NOTAM: Attempting to load cache from: %s", file_path.c_str());
  
  // Try to load from cache file
  if (LoadNOTAMsFromFile(cached_notams)) {
    const unsigned count = static_cast<unsigned>(cached_notams.size());
    
    // Store cached results in memory
    {
      const std::lock_guard<Mutex> lock(mutex);
      auto *impl = static_cast<NOTAMImpl*>(current_notams_impl);
      impl->current_notams = std::move(cached_notams);
    }
        
    LogFormat("NOTAM: Loaded %u NOTAMs from cache", count);
    return count;
  }
  
  LogFormat("NOTAM: No cached NOTAMs found at: %s", file_path.c_str());
  return 0;
}

void
NOTAMGlue::InvalidateCache()
{
  auto file_path = GetNOTAMCacheFilePath();
  
  if (File::Exists(file_path)) {
    LogFormat("NOTAM: Invalidating cache file: %s", file_path.c_str());
    File::Delete(file_path);
    LogFormat("NOTAM: Cache invalidated successfully");
  } else {
    LogFormat("NOTAM: No cache file to invalidate at: %s", file_path.c_str());
  }
}

int
NOTAMGlue::TestNOTAMFetch(const GeoPoint &location)
{
  // Launch the async fetch task 
  LoadNOTAMsInternal(location);
  
  // Return current count for immediate feedback
  const std::lock_guard<Mutex> lock(mutex);
  auto *impl = static_cast<NOTAMImpl*>(current_notams_impl);
  return static_cast<int>(impl->current_notams.size());
}

/**
 * Check if a NOTAM should be displayed based on filter settings
 */
static bool
ShouldDisplayNOTAM(const NOTAMStruct &notam, const NOTAMSettings &settings)
{
  LogFormat("NOTAM Filter: Checking %s - traffic='%s', text_preview='%.30s...', show_trigger=%d, show_traffic_ifr=%d",
            notam.number.c_str(), notam.traffic.c_str(), notam.text.c_str(),
            (int)settings.show_trigger, (int)settings.show_traffic_ifr);
  
  // Check traffic type filter (I=IFR, V=VFR, IV=both)
  if (!notam.traffic.empty()) {
    if (notam.traffic == "I" && !settings.show_traffic_ifr) {
      LogFormat("NOTAM Filter: %s is IFR-only (traffic=I), filtered out", notam.number.c_str());
      return false;
    }
    if (notam.traffic == "V" && !settings.show_traffic_vfr) {
      LogFormat("NOTAM Filter: %s is VFR-only (traffic=V), filtered out", notam.number.c_str());
      return false;
    }
    if (notam.traffic == "IV" && !settings.show_traffic_both) {
      LogFormat("NOTAM Filter: %s is IFR+VFR (traffic=IV), filtered out", notam.number.c_str());
      return false;
    }
  }
  
  // Check for TRIGGER NOTAM filter first (text-based)
  if (!settings.show_trigger) {
    // Check if "TRIGGER NOTAM" appears in the text field
    if (notam.text.find("TRIGGER NOTAM") != std::string::npos) {
      LogFormat("NOTAM Filter: %s contains 'TRIGGER NOTAM', filtered out by show_trigger=false",
                notam.number.c_str());
      return false;
    }
  }
  
  // feature_type actually contains the selectionCode (ICAO Q-code)
  const auto &qcode = notam.feature_type;
  
  if (qcode.empty()) {
    LogFormat("NOTAM Filter: %s has NO Q-code, treating as 'other' (show_other=%d)",
              notam.number.c_str(), (int)settings.show_other);
    return settings.show_other;
  }
  
  LogFormat("NOTAM Filter: %s Q-code='%s'", notam.number.c_str(), qcode.c_str());
  
  // Q-codes starting with QR = Restricted/Danger areas (airspace)
  // Q-codes starting with QD = Danger areas (airspace)
  if (qcode.length() >= 2 && (qcode.substr(0, 2) == "QR" || qcode.substr(0, 2) == "QD")) {
    return settings.show_airspace;
  }
  
  // Q-codes starting with QO = Obstacles
  if (qcode.length() >= 2 && qcode.substr(0, 2) == "QO") {
    return settings.show_obst;
  }
  
  // Q-codes starting with QW = Warnings (includes military exercises, UAS ops)
  if (qcode.length() >= 2 && qcode.substr(0, 2) == "QW") {
    return settings.show_military;
  }
  
  // Everything else (aerodromes, navaids, procedures, etc.)
  return settings.show_other;
}

void
NOTAMGlue::UpdateAirspaces(Airspaces &airspaces)
{
  const std::lock_guard<Mutex> lock(mutex);
  auto *impl = static_cast<NOTAMImpl*>(current_notams_impl);
  
  LogFormat("NOTAM: UpdateAirspaces - converting %u NOTAMs to airspaces", 
            static_cast<unsigned>(impl->current_notams.size()));

  // Save all non-NOTAM airspaces
  // ToDo: Optimize by removing only existing NOTAM airspaces
  std::vector<AirspacePtr> saved_airspaces;
  for (const auto &airspace : airspaces.QueryAll()) {
    tstring name = airspace.GetAirspace().GetName();
    // Skip airspaces with names starting with "NOTAM "
    if (name.find(_T("NOTAM ")) != 0) {
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
        std::string main_text = "NOTAM " + notam.number + ": " + notam.text;
        if (main_text.length() > 150) {
          main_text = main_text.substr(0, 147) + "...";
        }
        tstring notam_name = tstring(main_text.begin(), main_text.end());
        
        // Use ICAO location code for station name
        tstring notam_station = tstring(notam.location.begin(), notam.location.end());
        
        // Use the parsed AirspaceAltitude objects directly
        AirspaceAltitude base = notam.lower_altitude;
        AirspaceAltitude top = notam.upper_altitude;
        
        // Set reasonable defaults if altitudes are invalid (altitude == -1 indicates invalid)
        if (base.altitude < 0) {
          base.reference = AltitudeReference::AGL;
          base.altitude_above_terrain = 0;
          base.altitude = 0; // fallback
        }
        
        if (top.altitude < 0) {
          top.reference = AltitudeReference::MSL;
          top.altitude = 9999; // High altitude fallback
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
        
        LogFormat("NOTAM: Added airspace for NOTAM '%s' (type=%d, radius=%.0fm)", 
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
    LogFormat("NOTAM: Saving GeoJSON to cache: %s", file_path.c_str());
    
    // Create a wrapper object with metadata and the raw GeoJSON
    std::string json_with_metadata = "{";
    json_with_metadata += "\"xcsoar_timestamp\":" + std::to_string(std::time(nullptr)) + ",";
    json_with_metadata += "\"xcsoar_location_lat\":" + std::to_string(location.latitude.Degrees()) + ",";
    json_with_metadata += "\"xcsoar_location_lon\":" + std::to_string(location.longitude.Degrees()) + ",";
    json_with_metadata += "\"xcsoar_radius_km\":" + std::to_string(settings.radius_km) + ",";
    json_with_metadata += "\"xcsoar_refresh_interval_min\":" + std::to_string(settings.refresh_interval_min) + ",";
    json_with_metadata += "\"geojson\":" + geojson_response + "}";
    
    // Write to file (using XCSoar IO system) 
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
    LogFormat("NOTAM: LoadNOTAMsFromFile attempting to load: %s", file_path.c_str());
    
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
      LogFormat("NOTAM: LoadNOTAMsFromFile failed to open file: %s - %s", file_path.c_str(), e.what());
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
      if (current_location.IsValid()) {
        GeoPoint cached_location{Angle::Degrees(cached_lon), Angle::Degrees(cached_lat)};
        double distance_m = current_location.Distance(cached_location);
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
