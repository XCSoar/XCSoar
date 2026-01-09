// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The XCSoar Project

#include "Client.hpp"

#ifdef HAVE_HTTP
#include "NOTAM.hpp"
#include "Geo/GeoPoint.hpp"
#include "co/Task.hxx"
#include "lib/curl/CoRequest.hxx"
#include "lib/curl/Easy.hxx"
#include "lib/curl/Slist.hxx"
#include "lib/curl/Setup.hxx"
#include "lib/fmt/ToBuffer.hxx"
#include "Operation/ProgressListener.hpp"
#include "net/http/Progress.hpp"
#include "time/Convert.hxx"
#include "Engine/Airspace/AirspaceAltitude.hpp"
#include "io/StringOutputStream.hxx"
#include "json/Serialize.hxx"
#include "util/StringAPI.hxx"
#include "util/StringCompare.hxx"
#include "util/StringParser.hxx"
#include "util/CharUtil.hxx"
#include "util/ConvertString.hpp"
#include "util/StringStrip.hxx"
#include "Units/System.hpp"
#include "Units/Descriptor.hpp"
#include "Geo/AltitudeReference.hpp"
#include "LogFile.hpp"

#include <boost/json.hpp>
#include <stdexcept>
#include <ctime>
#include <algorithm>

using std::string_view_literals::operator""sv;

namespace {
constexpr double NM_TO_METERS = 1852.0;
constexpr double DEFAULT_NOTAM_RADIUS_METERS = 5000.0;
}

/**
 * Parse altitude string to AirspaceAltitude using XCSoar's altitude parsing
 * logic
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
    altitude.flight_level = 0;
    altitude.altitude_above_terrain = 0;
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

static AirspaceAltitude
ParseFlightLevelLimit(const std::string &fl_value)
{
  const auto trimmed = Strip(std::string_view{fl_value});
  if (trimmed.empty()) {
    return ParseAltitudeString("");
  }

  if (trimmed == "000"sv) {
    return ParseAltitudeString("GND");
  }

  if (trimmed == "999"sv) {
    return ParseAltitudeString("UNL");
  }

  std::string fl;
  fl.reserve(2 + trimmed.size());
  fl.append("FL");
  fl.append(trimmed);
  return ParseAltitudeString(fl);
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
    return NOTAMTime::PermanentEndTime();
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
  
  throw std::runtime_error("Failed to parse ISO8601 timestamp: " + iso_string);
}

static void
ParseNOTAMProperties(NOTAM &notam, const boost::json::object &notam_obj)
{
  bool has_lower_limit = false;
  bool has_upper_limit = false;

  notam.lower_altitude = ParseAltitudeString("");
  notam.upper_altitude = ParseAltitudeString("");

  // Parse NOTAM properties from the nested object
  if (auto it = notam_obj.find("id"); it != notam_obj.end()) {
    notam.id = boost::json::value_to<std::string>(it->value());
  }

  if (auto it = notam_obj.find("lastUpdated"); it != notam_obj.end()) {
    notam.last_updated = boost::json::value_to<std::string>(it->value());
  }
  
  // Parse NOTAM number
  if (auto it = notam_obj.find("number"); it != notam_obj.end()) {
    notam.number = boost::json::value_to<std::string>(it->value());
  }
  
  if (auto it = notam_obj.find("text"); it != notam_obj.end()) {
    notam.text = boost::json::value_to<std::string>(it->value());
  }
  
  if (auto it = notam_obj.find("effectiveStart"); it != notam_obj.end()) {
    notam.start_time =
      ParseISO8601(boost::json::value_to<std::string>(it->value()));
  }
  
  if (auto it = notam_obj.find("effectiveEnd"); it != notam_obj.end()) {
    notam.end_time =
      ParseISO8601(boost::json::value_to<std::string>(it->value()));
  }
  
  // Parse altitude limits using airspace altitude parsing system
  if (auto it = notam_obj.find("lowerLimit"); it != notam_obj.end()) {
    const auto lower_str = boost::json::value_to<std::string>(it->value());
    notam.lower_altitude = ParseAltitudeString(lower_str);
    has_lower_limit = true;
  }
  
  if (auto it = notam_obj.find("upperLimit"); it != notam_obj.end()) {
    const auto upper_str = boost::json::value_to<std::string>(it->value());
    notam.upper_altitude = ParseAltitudeString(upper_str);
    has_upper_limit = true;
  }
  
  if (auto it = notam_obj.find("classification"); it != notam_obj.end()) {
    notam.classification = boost::json::value_to<std::string>(it->value());
  }
  
  // Parse NOTAM series (e.g., F, M, B, W)
  if (auto it = notam_obj.find("series"); it != notam_obj.end()) {
    notam.series = boost::json::value_to<std::string>(it->value());
  }
  
  // Parse NOTAM type (R=Replace, N=New, C=Cancel)
  if (auto it = notam_obj.find("type"); it != notam_obj.end()) {
    notam.type = boost::json::value_to<std::string>(it->value());
  }
  
  // Parse selection code (ICAO Q-code that indicates NOTAM type)
  if (auto it = notam_obj.find("selectionCode"); it != notam_obj.end()) {
    notam.feature_type = boost::json::value_to<std::string>(it->value());
  }
  
  // Parse flight level limits
  if (auto it = notam_obj.find("minimumFL"); it != notam_obj.end()) {
    notam.minimum_fl = boost::json::value_to<std::string>(it->value());
    if (!has_lower_limit)
      notam.lower_altitude = ParseFlightLevelLimit(notam.minimum_fl);
  }
  
  if (auto it = notam_obj.find("maximumFL"); it != notam_obj.end()) {
    notam.maximum_fl = boost::json::value_to<std::string>(it->value());
    if (!has_upper_limit)
      notam.upper_altitude = ParseFlightLevelLimit(notam.maximum_fl);
  }
  
  // Parse location field (e.g., related airport ICAO code)
  if (auto it = notam_obj.find("location"); it != notam_obj.end()) {
    notam.location = boost::json::value_to<std::string>(it->value());
  }
  
  // Parse traffic type (I=IFR, V=VFR, IV=both)
  if (auto it = notam_obj.find("traffic"); it != notam_obj.end()) {
    notam.traffic = boost::json::value_to<std::string>(it->value());
  }
  
  // Use icaoLocation as source
  if (auto it = notam_obj.find("icaoLocation"); it != notam_obj.end()) {
    notam.source = boost::json::value_to<std::string>(it->value());
  }
}

static void
ParseNOTAMGeometry(NOTAM &notam, const boost::json::object &feature,
                   const boost::json::object &notam_obj)
{
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
          const auto radius_str =
            boost::json::value_to<std::string>(it->value());
          try {
            // Radius is typically in nautical miles, convert to meters
            double radius_nm = std::stod(radius_str);
            notam.geometry.radius_meters = radius_nm * NM_TO_METERS;
          } catch (const std::exception &) {
            LogFormat("Failed to parse NOTAM radius for %s: %s",
                      notam.number.c_str(), radius_str.c_str());
            notam.geometry.radius_meters = DEFAULT_NOTAM_RADIUS_METERS;
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
      if (!ring.empty()) {
        // First point as reference
        notam.geometry.center =
          boost::json::value_to<NOTAMPoint>(ring[0]);
        
        for (const auto &coord : ring) {
          notam.geometry.polygon_points.emplace_back(
            boost::json::value_to<NOTAMPoint>(coord)
          );
        }
      }
    }
  }
}

static NOTAM
ParseNOTAMFeature(const boost::json::object &feature)
{
  NOTAM notam;

  // Get the nested NOTAM data
  const auto &props = feature.at("properties").as_object();
  const auto &core_notam_data = props.at("coreNOTAMData").as_object();
  const auto &notam_obj = core_notam_data.at("notam").as_object();

  ParseNOTAMProperties(notam, notam_obj);
  ParseNOTAMGeometry(notam, feature, notam_obj);

  return notam;
}

static std::vector<NOTAM>
ParseNOTAMItems(const boost::json::array &items)
{
  std::vector<NOTAM> notams;
  notams.reserve(items.size());

  for (const auto &item_val : items) {
    const auto &feature = item_val.as_object();
    try {
      notams.emplace_back(ParseNOTAMFeature(feature));
    } catch (const std::exception &e) {
      // Skip invalid NOTAMs but continue processing others
      LogDebug("NOTAM: Skipped invalid item: %s", e.what());
      continue;
    }
  }
  
  return notams;
}

std::vector<NOTAM>
ParseNOTAMGeoJSON(const boost::json::value &json)
{
  const auto &root = json.as_object();

  // The API returns items array instead of features
  const auto &items = root.at("items").as_array();

  return ParseNOTAMItems(items);
}

static NOTAMResponse
ParseNOTAMResponse(const boost::json::value &json)
{
  NOTAMResponse response;

  const auto &root = json.as_object();
  if (auto it = root.find("delta");
      it != root.end() && it->value().is_bool()) {
    response.is_delta = it->value().as_bool();
  }

  if (auto it = root.find("removedIds");
      it != root.end() && it->value().is_array()) {
    const auto &removed = it->value().as_array();
    response.removed_ids.reserve(removed.size());
    for (const auto &entry : removed) {
      if (entry.is_string())
        response.removed_ids.emplace_back(entry.as_string().c_str());
    }
  }

  if (auto it = root.find("items");
      it != root.end() && it->value().is_array()) {
    response.notams = ParseNOTAMItems(it->value().as_array());
  }

  return response;
}

std::vector<NOTAM>
ParseNOTAMGeoJSON(const std::string &json_data)
{
  auto json_value = boost::json::parse(json_data);
  return ParseNOTAMGeoJSON(json_value);
}

Co::Task<NOTAMResponse>
FetchNOTAMsResponse(CurlGlobal &curl, const NOTAMSettings &settings,
                    const GeoPoint &location, ProgressListener &progress,
                    const KnownMap *known)
{
  // Build URL with parameters - convert wide string to UTF-8 for fmt
  WideToUTF8Converter base_url_conv(settings.api_base_url);
  if (!base_url_conv.IsValid()) {
    LogFormat("NOTAM: Failed to convert API base URL to UTF-8");
    co_return NOTAMResponse{};
  }
  const double raw_radius_nm = (settings.radius_km * 1000.0) / NM_TO_METERS;
  constexpr double MAX_RADIUS_NM = 100.0;
  const double radius_nm = std::clamp(raw_radius_nm, 0.0, MAX_RADIUS_NM);
  if (raw_radius_nm > MAX_RADIUS_NM)
    LogFormat("NOTAM: Requested radius %.1f NM clamped to %.1f NM",
              raw_radius_nm, MAX_RADIUS_NM);
  const auto url = fmt::format(
    "{}?locationLongitude={}&locationLatitude={}&locationRadius={:.2f}",
    base_url_conv.c_str(),
    location.longitude.Degrees(),
    location.latitude.Degrees(),
    radius_nm);

  LogFormat("NOTAM: Fetching NOTAMs within %.1f NM radius", radius_nm);
  LogDebug("NOTAM: Request URL: %s", url.c_str());

  CurlEasy easy(url.c_str());
  Curl::Setup(easy);

  CurlSlist headers;
  std::string request_body;
  if (known != nullptr && !known->empty()) {
    boost::json::object known_obj;
    for (const auto &entry : *known) {
      if (!entry.first.empty() && !entry.second.empty())
        known_obj[entry.first] = entry.second;
    }

    if (!known_obj.empty()) {
      boost::json::object payload;
      payload["known"] = std::move(known_obj);
      StringOutputStream sos;
      Json::Serialize(sos, boost::json::value(std::move(payload)));
      request_body = std::move(sos).GetValue();
      easy.SetPost();
      easy.SetRequestBody(request_body);
      headers.Append("Content-Type: application/json");
      easy.SetRequestHeaders(headers.Get());
    }
  }

  const Net::ProgressAdapter progress_adapter{easy, progress};

  const auto http_response =
    co_await Curl::CoRequest(curl, std::move(easy));

  LogFormat("NOTAM: Response status=%u, body=%lu bytes",
            http_response.status, (unsigned long)http_response.body.size());

  if (http_response.status != 200) {
    if (!http_response.body.empty()) {
      constexpr size_t max_snippet = 512;
      std::string snippet = http_response.body.substr(0, max_snippet);
      LogFormat("NOTAM: Response body (first %lu bytes): %s",
                (unsigned long)snippet.size(), snippet.c_str());
    }
    throw std::runtime_error(
      fmt::format("Failed to fetch NOTAMs: HTTP {}", http_response.status));
  }

  auto json_value = boost::json::parse(http_response.body);
  auto parsed = ParseNOTAMResponse(json_value);
  parsed.raw_json = http_response.body;
  co_return parsed;
}

} // namespace NOTAMClient

#endif // HAVE_HTTP
