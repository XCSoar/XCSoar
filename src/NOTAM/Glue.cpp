// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The XCSoar Project

#include "Glue.hpp"
#include "NOTAM.hpp"
#include "Client.hpp"

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
NOTAMGlue::UpdateLocation(const GeoPoint &location)
{
  const std::lock_guard<Mutex> lock(mutex);
  
  // Check if location changed significantly (more than 10km)
  if (current_location.IsValid() && location.IsValid()) {
    const auto distance_m = current_location.Distance(location);
    constexpr double RELOAD_THRESHOLD_M = 10000.0; // 10km threshold
    if (distance_m < RELOAD_THRESHOLD_M) {
      return; // No significant change
    }
  }
  
  current_location = location;
  
  // Trigger reload if not already loading and settings are enabled
  if (!loading && settings.enabled && location.IsValid()) {
    // @TODO
    // We can't directly call LoadNOTAMs from here as we don't have OperationEnvironment
    // Could be handled by a timer / the main application loop
    // For now, we just update the location and let external code trigger the reload
  }
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
  try {
    // First, try to load from cache if not expired
    if (!IsCacheExpired(location)) {
      std::vector<NOTAMStruct> cached_notams;
      if (LoadNOTAMsFromFile(location, cached_notams)) {
        // Store cached results
        {
          const std::lock_guard<Mutex> lock(mutex);
          auto *impl = static_cast<NOTAMImpl*>(current_notams_impl);
          impl->current_notams = std::move(cached_notams);
        }
        co_return; // Use cached data, no need to fetch
      }
    }
    
    // Cache is expired or doesn't exist, fetch fresh data from API
    // Use a simple progress listener (we could make this more sophisticated)
    class SimpleProgressListener : public ProgressListener {
    public:
      void SetProgressRange(unsigned range) noexcept override { (void)range; }
      void SetProgressPosition(unsigned position) noexcept override { (void)position; }
    } progress;
    
    // Fetch raw GeoJSON using the client
    auto raw_geojson = co_await NOTAMClient::FetchNOTAMsRaw(curl, settings, location, progress);
    
    // Save raw GeoJSON to file for caching
    SaveNOTAMsToFile(location, raw_geojson);
    
    // Parse GeoJSON to get NOTAMs for in-memory storage
    auto notams = NOTAMClient::ParseNOTAMGeoJSON(raw_geojson);
    
    // Store the results
    {
      const std::lock_guard<Mutex> lock(mutex);
      auto *impl = static_cast<NOTAMImpl*>(current_notams_impl);
      impl->current_notams = std::move(notams);
    }
    

    
  } catch (const std::exception &e) {
    // Log error and clear any existing NOTAMs
    {
      const std::lock_guard<Mutex> lock(mutex);
      auto *impl = static_cast<NOTAMImpl*>(current_notams_impl);
      impl->current_notams.clear();
    }
    // Could log the error: e.what()
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
  
  // TODO: Handle error if needed
  (void)error;
}

unsigned
NOTAMGlue::LoadCachedNOTAMs(const GeoPoint &location)
{
  std::vector<NOTAMStruct> cached_notams;
  
  // Log the cache file path for debugging
  auto file_path = GetNOTAMCacheFilePath(location);
  LogFormat("NOTAM: Attempting to load cache from: %s", file_path.c_str());
  
  // Try to load from cache file
  if (LoadNOTAMsFromFile(location, cached_notams)) {
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

void
NOTAMGlue::UpdateAirspaces(Airspaces &airspaces)
{
  const std::lock_guard<Mutex> lock(mutex);
  auto *impl = static_cast<NOTAMImpl*>(current_notams_impl);
  
  LogFormat("NOTAM: UpdateAirspaces - converting %u NOTAMs to airspaces", 
            static_cast<unsigned>(impl->current_notams.size()));

  // The caller is responsible for ensuring the airspace database
  // is in the correct state before calling this method (e.g., cleared or fresh).
  // This method only ADDS airspaces, it does not remove existing ones.
  
  for (const auto &notam : impl->current_notams) {
    try {
      // Create airspace based on NOTAM geometry
      AirspacePtr airspace_ptr;
      
      switch (notam.geometry.type) {
        case NOTAM::NOTAMGeometry::Type::CIRCLE: {
          auto circle = std::make_shared<AirspaceCircle>(
            GeoPoint{Angle::Degrees(notam.geometry.center.longitude), 
                     Angle::Degrees(notam.geometry.center.latitude)},
            notam.geometry.radius_meters);
          // For debugging, drop all NOTAMs with radius > 50 km to avoid cluttering @TODO
          if (notam.geometry.radius_meters > 50000.0) {
            LogFormat("NOTAM: Skipping NOTAM '%s' with large radius %.0fm to avoid cluttering TODO FIX ME with proper filtering", 
                      notam.number.c_str(), notam.geometry.radius_meters);
            continue;
          }
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
  
  LogFormat("NOTAM: UpdateAirspaces completed");
}

AllocatedPath
NOTAMGlue::GetNOTAMCacheFilePath(const GeoPoint &location) const
{
  // Use fixed filename in main XCSoarData directory
  // We only have one cache file to KISS, no merging, no modifications
  (void)location; // Suppress unused parameter warning @TODO
  return LocalPath(_T("notams.json"));
}

void
NOTAMGlue::SaveNOTAMsToFile(const GeoPoint &location, const std::string &geojson_response) const
{
  try {
    auto file_path = GetNOTAMCacheFilePath(location);
    LogFormat("NOTAM: Saving GeoJSON to cache: %s", file_path.c_str());
    
    // Create a wrapper object with metadata and the raw GeoJSON
    std::string json_with_metadata = "{";
    json_with_metadata += "\"xcsoar_timestamp\":" + std::to_string(std::time(nullptr)) + ",";
    json_with_metadata += "\"xcsoar_location_lat\":" + std::to_string(location.latitude.Degrees()) + ",";
    json_with_metadata += "\"xcsoar_location_lon\":" + std::to_string(location.longitude.Degrees()) + ",";
    json_with_metadata += "\"xcsoar_radius_km\":" + std::to_string(settings.radius_km) + ",";
    json_with_metadata += "\"xcsoar_refresh_interval_min\":" + std::to_string(settings.refresh_interval_min.count()) + ",";
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
NOTAMGlue::LoadNOTAMsFromFile(const GeoPoint &location, std::vector<NOTAMStruct> &notams) const
{
  try {
    auto file_path = GetNOTAMCacheFilePath(location);
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

bool
NOTAMGlue::IsCacheExpired(const GeoPoint &location) const
{
  try {
    auto file_path = GetNOTAMCacheFilePath(location);
    
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
            std::time_t cached_time = std::stoll(timestamp_str);
            std::time_t current_time = std::time(nullptr);
            
            // Convert refresh interval from minutes to seconds
            std::time_t max_age_seconds = settings.refresh_interval_min.count() * 60;
            
            return (current_time - cached_time) > max_age_seconds;
          } catch (const std::exception &) {
            return true; // Invalid timestamp means expired
          }
        }
      }
      
      return true; // No timestamp found means expired
    } catch (const std::exception &e) {
      return true; // Error means treat as expired
    }
  } catch (const std::exception &e) {
    return true; // Error means treat as expired
  }
}

#endif // HAVE_HTTP
