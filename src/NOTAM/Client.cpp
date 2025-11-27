// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The XCSoar Project

#include "Client.hpp"

#ifdef HAVE_HTTP 
#include "NOTAM.hpp"
#include "Geo/GeoPoint.hpp"
#include "co/Task.hxx"
#include "lib/curl/CoRequest.hxx"
#include "lib/curl/Easy.hxx"
#include "lib/curl/Setup.hxx"
#include "lib/fmt/ToBuffer.hxx"
#include "Operation/ProgressListener.hpp"
#include "net/http/Progress.hpp"
#include "time/Convert.hxx"
#include "Engine/Airspace/AirspaceAltitude.hpp"
#include "util/StringAPI.hxx"
#include "util/StringCompare.hxx"
#include "util/StringParser.hxx"
#include "util/CharUtil.hxx"
#include "Units/System.hpp"
#include "Units/Descriptor.hpp"
#include "Geo/AltitudeReference.hpp"

#include <boost/json.hpp>
#include <stdexcept>
#include <ctime>

using std::string_view_literals::operator""sv;

/**
 * Parse altitude string to AirspaceAltitude using XCSoar's altitude parsing logic
 * Based on ReadAltitude from AirspaceParser.cpp
 */
static AirspaceAltitude
ParseAltitudeString(const std::string &alt_str)
{
  if (alt_str.empty()) {
    // Return unknown/invalid altitude
    AirspaceAltitude altitude;
    altitude.reference = AltitudeReference::MSL;
    altitude.altitude = -1.0; // Invalid marker
    return altitude;
  }

  // Create a StringParser from our string
  StringParser<> input{alt_str.c_str()};
  
  auto unit = Unit::FEET;
  enum { MSL, AGL, SFC, FL, STD, UNLIMITED } type = MSL;
  double value = 0;

  while (true) {
    input.Strip();

    if (IsDigitASCII(input.front())) {
      if (auto x = input.ReadDouble())
        value = *x;
    } else if (input.SkipMatchIgnoreCase("GND"sv) ||
               input.SkipMatchIgnoreCase("AGL"sv)) {
      type = AGL;
    } else if (input.SkipMatchIgnoreCase("SFC"sv)) {
      type = SFC;
    } else if (input.SkipMatchIgnoreCase("FL"sv)) {
      type = FL;
    } else if (input.SkipMatchIgnoreCase("FT"sv)) {
      unit = Unit::FEET;
    } else if (input.SkipMatchIgnoreCase("MSL"sv)) {
      type = MSL;
    } else if (input.front() == 'M' || input.front() == 'm') {
      unit = Unit::METER;
      input.Skip();
    } else if (input.SkipMatchIgnoreCase("STD"sv)) {
      type = STD;
    } else if (input.SkipMatchIgnoreCase("UNL"sv)) {
      type = UNLIMITED;
    } else if (input.IsEmpty())
      break;
    else
      input.Skip();
  }

  AirspaceAltitude altitude;

  switch (type) {
  case FL:
    altitude.reference = AltitudeReference::STD;
    altitude.flight_level = value;
    /* prepare fallback, just in case we have no terrain */
    altitude.altitude = Units::ToSysUnit(value, Unit::FLIGHT_LEVEL);
    return altitude;

  case UNLIMITED:
    altitude.reference = AltitudeReference::MSL;
    altitude.altitude = 50000;
    return altitude;

  case SFC:
    altitude.reference = AltitudeReference::AGL;
    altitude.altitude_above_terrain = -1;
    /* prepare fallback, just in case we have no terrain */
    altitude.altitude = 0;
    return altitude;

  default:
    break;
  }

  // For MSL, AGL and STD we convert the altitude to meters
  value = Units::ToSysUnit(value, unit);
  switch (type) {
  case MSL:
    altitude.reference = AltitudeReference::MSL;
    altitude.altitude = value;
    return altitude;

  case AGL:
    altitude.reference = AltitudeReference::AGL;
    altitude.altitude_above_terrain = value;
    /* prepare fallback, just in case we have no terrain */
    altitude.altitude = value;
    return altitude;

  case STD:
    altitude.reference = AltitudeReference::STD;
    altitude.flight_level = Units::ToUserUnit(value, Unit::FLIGHT_LEVEL);
    /* prepare fallback, just in case we have no QNH */
    altitude.altitude = value;
    return altitude;

  default:
    // Default to MSL
    altitude.reference = AltitudeReference::MSL;
    altitude.altitude = value;
    return altitude;
  }
}

static auto
tag_invoke(boost::json::value_to_tag<NOTAMPoint>,
           const boost::json::value &jv)
{
  if (jv.is_array()) {
    const auto &arr = jv.as_array();
    if (arr.size() >= 2) {
      return NOTAMPoint{
        arr[0].to_number<double>(), // longitude
        arr[1].to_number<double>()  // latitude  
      };
    }
  }
  throw std::runtime_error("Invalid coordinate format");
}

namespace NOTAMClient {

static std::chrono::system_clock::time_point
ParseISO8601(const std::string &iso_string)
{
  // Parse ISO8601 format: YYYY-MM-DDTHH:MM:SS.sssZ or PERM
  if (iso_string == "PERM") {
    // Return a far future date for permanent NOTAMs
    return std::chrono::system_clock::time_point::max();
  }
  
  struct tm tm = {};
  const char* str = iso_string.c_str();
  
  // Parse YYYY-MM-DDTHH:MM:SS format
  int year, month, day, hour, min, sec;
  int scanned = sscanf(str, "%d-%d-%dT%d:%d:%d", 
                       &year, &month, &day, &hour, &min, &sec);
  
  if (scanned >= 6) {
    tm.tm_year = year - 1900;
    tm.tm_mon = month - 1;
    tm.tm_mday = day;
    tm.tm_hour = hour;
    tm.tm_min = min;
    tm.tm_sec = sec;
    tm.tm_isdst = 0;  // UTC time
    
    return TimeGm(tm);
  }
  
  // Fallback to current time if parsing fails
  return std::chrono::system_clock::now();
}

static NOTAM
ParseNOTAMFeature(const boost::json::object &feature)
{
  NOTAM notam;
  
  // Get the nested NOTAM data
  const auto &props = feature.at("properties").as_object();
  const auto &core_notam_data = props.at("coreNOTAMData").as_object();
  const auto &notam_obj = core_notam_data.at("notam").as_object();
  
  // Parse NOTAM properties from the nested object
  if (auto it = notam_obj.find("id"); it != notam_obj.end()) {
    notam.id = boost::json::value_to<std::string>(it->value());
  }
  
  // Parse NOTAM number
  if (auto it = notam_obj.find("number"); it != notam_obj.end()) {
    notam.number = boost::json::value_to<std::string>(it->value());
  }
  
  if (auto it = notam_obj.find("text"); it != notam_obj.end()) {
    notam.text = boost::json::value_to<std::string>(it->value());
  }
  
  if (auto it = notam_obj.find("effectiveStart"); it != notam_obj.end()) {
    notam.start_time = ParseISO8601(boost::json::value_to<std::string>(it->value()));
  }
  
  if (auto it = notam_obj.find("effectiveEnd"); it != notam_obj.end()) {
    notam.end_time = ParseISO8601(boost::json::value_to<std::string>(it->value()));
  }
  
  // Parse altitude limits using airspace altitude parsing system
  if (auto it = notam_obj.find("lowerLimit"); it != notam_obj.end()) {
    const auto lower_str = boost::json::value_to<std::string>(it->value());
    notam.lower_altitude = ParseAltitudeString(lower_str);
  }
  
  if (auto it = notam_obj.find("upperLimit"); it != notam_obj.end()) {
    const auto upper_str = boost::json::value_to<std::string>(it->value());
    notam.upper_altitude = ParseAltitudeString(upper_str);
  }
  
  if (auto it = notam_obj.find("classification"); it != notam_obj.end()) {
    notam.classification = boost::json::value_to<std::string>(it->value());
  }
  
  // Parse selection code (ICAO Q-code that indicates NOTAM type)
  if (auto it = notam_obj.find("selectionCode"); it != notam_obj.end()) {
    notam.feature_type = boost::json::value_to<std::string>(it->value());
  }
  
  // Parse flight level limits
  if (auto it = notam_obj.find("minimumFL"); it != notam_obj.end()) {
    notam.minimum_fl = boost::json::value_to<std::string>(it->value());
  }
  
  if (auto it = notam_obj.find("maximumFL"); it != notam_obj.end()) {
    notam.maximum_fl = boost::json::value_to<std::string>(it->value());
  }
  
  // Parse location field (e.g., related airport ICAO code)
  if (auto it = notam_obj.find("location"); it != notam_obj.end()) {
    notam.location = boost::json::value_to<std::string>(it->value());
  }
  
  // Use icaoLocation as source
  if (auto it = notam_obj.find("icaoLocation"); it != notam_obj.end()) {
    notam.source = boost::json::value_to<std::string>(it->value());
  }
  
  // Parse geometry - the API uses GeometryCollection with Point geometries
  const auto &geometry = feature.at("geometry").as_object();
  const auto &geom_type = geometry.at("type").as_string();
  
  if (geom_type == "GeometryCollection"sv) {
    const auto &geometries = geometry.at("geometries").as_array();
    if (!geometries.empty()) {
      const auto &first_geom = geometries[0].as_object();
      const auto &first_type = first_geom.at("type").as_string();
      
      if (first_type == "Point"sv) {
        notam.geometry.type = NOTAM::NOTAMGeometry::Type::POINT;
        const auto &coords = first_geom.at("coordinates");
        notam.geometry.center = boost::json::value_to<NOTAMPoint>(coords);
        
        // Check if there's a radius in the NOTAM data to make it a circle
        if (auto it = notam_obj.find("radius"); it != notam_obj.end()) {
          notam.geometry.type = NOTAM::NOTAMGeometry::Type::CIRCLE;
          const auto radius_str = boost::json::value_to<std::string>(it->value());
          try {
            // Radius is typically in nautical miles, convert to meters
            double radius_nm = std::stod(radius_str);
            notam.geometry.radius_meters = radius_nm * 1852.0; // NM to meters
          } catch (...) {
            notam.geometry.radius_meters = 5000.0; // Default 5km radius
          }
        }
      }
    }
  } else if (geom_type == "Point"sv) {
    notam.geometry.type = NOTAM::NOTAMGeometry::Type::POINT;
    const auto &coords = geometry.at("coordinates");
    notam.geometry.center = boost::json::value_to<NOTAMPoint>(coords);
  } else if (geom_type == "Polygon"sv) {
    notam.geometry.type = NOTAM::NOTAMGeometry::Type::POLYGON;
    const auto &coords = geometry.at("coordinates").as_array();
    if (!coords.empty()) {
      const auto &ring = coords[0].as_array(); // First ring (exterior)
      notam.geometry.center = boost::json::value_to<NOTAMPoint>(ring[0]); // First point as reference
      
      for (const auto &coord : ring) {
        notam.geometry.polygon_points.emplace_back(
          boost::json::value_to<NOTAMPoint>(coord)
        );
      }
    }
  }
  
  return notam;
}

static std::vector<NOTAM>
ParseNOTAMGeoJSON(const boost::json::value &json)
{
  std::vector<NOTAM> notams;
  
  const auto &root = json.as_object();
  
  // The API returns items array instead of features
  const auto &items = root.at("items").as_array();
  
  for (const auto &item_val : items) {
    const auto &feature = item_val.as_object();
    try {
      notams.emplace_back(ParseNOTAMFeature(feature));
    } catch (const std::exception &e) {
      // Skip invalid NOTAMs but continue processing others
      // Could log the error here if needed: e.what()
      continue;
    }
  }
  
  return notams;
}

std::vector<NOTAM>
ParseNOTAMGeoJSON(const std::string &json_data)
{
  auto json_value = boost::json::parse(json_data);
  return ParseNOTAMGeoJSON(json_value);
}

Co::Task<std::string>
FetchNOTAMsRaw(CurlGlobal &curl, const NOTAMSettings &settings,
               const GeoPoint &location, ProgressListener &progress)
{
  // Build URL with parameters
  const auto url = fmt::format("{}?locationLongitude={}&locationLatitude={}&locationRadius={}&pageSize={}",
    settings.api_base_url,
    location.longitude.Degrees(),
    location.latitude.Degrees(),
    static_cast<int>(settings.radius_km),
    settings.max_notams
  );
  
  CurlEasy easy{url.c_str()};
  Curl::Setup(easy);
  const Net::ProgressAdapter progress_adapter{easy, progress};
  
  const auto response = 
    co_await Curl::CoRequest(curl, std::move(easy));
  
  if (response.status != 200) {
    throw std::runtime_error("Failed to fetch NOTAMs");
  }
  
  // Return raw JSON response
  co_return response.body;
}

Co::Task<std::vector<NOTAM>>
FetchNOTAMs(CurlGlobal &curl, const NOTAMSettings &settings,
            const GeoPoint &location, ProgressListener &progress)
{
  // Get raw JSON first
  const auto raw_json = co_await FetchNOTAMsRaw(curl, settings, location, progress);
  
  // Parse JSON from response body
  auto body = boost::json::parse(raw_json);
  co_return ParseNOTAMGeoJSON(body);
}

} // namespace NOTAMClient

#endif // HAVE_HTTP