// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The XCSoar Project

#include "Client.hpp"

#ifdef HAVE_HTTP
#include "Config.hpp"
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
#include "time/Calendar.hxx"
#include "time/Convert.hxx"
#include "Engine/Airspace/AirspaceAltitude.hpp"
#include "io/StringOutputStream.hxx"
#include "json/Serialize.hxx"
#include "util/StringAPI.hxx"
#include "util/StringCompare.hxx"
#include "util/StringParser.hxx"
#include "util/CharUtil.hxx"
#include "util/StringStrip.hxx"
#include "util/UTF8.hpp"
#include "Units/System.hpp"
#include "Units/Descriptor.hpp"
#include "Geo/AltitudeReference.hpp"
#include "LogFile.hpp"

#include <boost/json.hpp>
#include <algorithm>
#include <cmath>
#include <cstdio>
#include <cstring>
#include <ctime>
#include <iterator>
#include <stdexcept>

using std::string_view_literals::operator""sv;

namespace {
constexpr double NM_TO_METERS = 1852.0;
constexpr double DEFAULT_NOTAM_RADIUS_METERS = 5000.0;

static void
ValidateCoordinate(double value, double min_value, double max_value,
                   const char *name)
{
  if (!std::isfinite(value))
    throw std::runtime_error(std::string("Invalid ") + name +
                             " coordinate: not finite");

  if (value < min_value || value > max_value)
    throw std::runtime_error(std::string("Invalid ") + name +
                             " coordinate range");
}

[[gnu::const]]
static double
NormalizeLongitude(double longitude) noexcept
{
  while (longitude < -180)
    longitude += 360;

  while (longitude > 180)
    longitude -= 360;

  return longitude;
}

[[gnu::const]]
static double
UnwrapLongitude(double longitude, double reference) noexcept
{
  while (longitude - reference > 180)
    longitude -= 360;

  while (longitude - reference < -180)
    longitude += 360;

  return longitude;
}
}

[[gnu::pure]]
static GeoPoint
ToGeoPoint(const NOTAMPoint &point) noexcept
{
  return GeoPoint{Angle::Degrees(point.longitude),
                  Angle::Degrees(point.latitude)};
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
    AirspaceAltitude altitude{};
    altitude.reference = AltitudeReference::MSL;
    altitude.altitude = NOTAMAltitude::INVALID_ALTITUDE;
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

    if (input.IsEmpty())
      break;

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
    } else if (input.SkipMatchIgnoreCase("AMSL"sv) ||
               input.SkipMatchIgnoreCase("MSL"sv)) {
      type = MSL;
    } else if (input.front() == 'M' || input.front() == 'm') {
      unit = Unit::METER;
      input.Skip();
    } else if (input.SkipMatchIgnoreCase("STD"sv)) {
      type = STD;
    } else if (input.SkipMatchIgnoreCase("UNL"sv)) {
      type = UNLIMITED;
    } else {
      LogFmt("NOTAM: Unexpected altitude token '{}' in '{}'",
             input.front(), alt_str.c_str());
      break;
    }
  }

  AirspaceAltitude altitude{};

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
      const double longitude = arr[0].to_number<double>();
      const double latitude = arr[1].to_number<double>();

      ValidateCoordinate(longitude, -180., 180., "longitude");
      ValidateCoordinate(latitude, -90., 90., "latitude");

      return NOTAMPoint{
        longitude,
        latitude,
      };
    }
  }
  throw std::runtime_error("Invalid coordinate format");
}

namespace NOTAMClient {

static std::chrono::system_clock::time_point
ParseISO8601(const std::string &iso_string, const bool allow_permanent)
{
  // Parse ISO8601 format: YYYY-MM-DDTHH:MM:SS.sssZ or PERM
  if (iso_string == "PERM") {
    if (!allow_permanent)
      throw std::runtime_error("Invalid ISO8601 timestamp '" + iso_string +
                               "': PERM is only valid as an end time");

    // Return a far future date for permanent NOTAMs
    return NOTAMTime::PermanentEndTime();
  }
  
  struct tm tm = {};
  const char* str = iso_string.c_str();
  
  // Parse YYYY-MM-DDTHH:MM:SS format
  int year = 0, month = 0, day = 0, hour = 0, min = 0, sec = 0;
  int consumed = 0;
  const int scanned = sscanf(str, "%d-%d-%dT%d:%d:%d%n",
                       &year, &month, &day, &hour, &min, &sec,
                       &consumed);
  
  if (scanned >= 6) {
    if (year < 0)
      throw std::runtime_error("Invalid ISO8601 timestamp '" + iso_string +
                               "': year out of range");

    if (month < 1 || month > 12)
      throw std::runtime_error("Invalid ISO8601 timestamp '" + iso_string +
                               "': month out of range");

    if (day < 1 || day > 31)
      throw std::runtime_error("Invalid ISO8601 timestamp '" + iso_string +
                               "': day out of range");

    if (static_cast<unsigned>(day) > DaysInMonth(month, year))
      throw std::runtime_error("Invalid ISO8601 timestamp '" + iso_string +
                               "': impossible date");

    if (hour < 0 || hour > 23)
      throw std::runtime_error("Invalid ISO8601 timestamp '" + iso_string +
                               "': hour out of range");

    if (min < 0 || min > 59)
      throw std::runtime_error("Invalid ISO8601 timestamp '" + iso_string +
                               "': minute out of range");

    if (sec < 0 || sec > 59)
      throw std::runtime_error("Invalid ISO8601 timestamp '" + iso_string +
                               "': second out of range");

    const char *suffix = str + consumed;
    if (*suffix == '.') {
      ++suffix;
      if (!IsDigitASCII(*suffix))
        throw std::runtime_error("Invalid ISO8601 timestamp '" + iso_string +
                                 "': non-UTC timezone or extra characters");

      while (IsDigitASCII(*suffix))
        ++suffix;
    }

    if (*suffix != '\0' && !(suffix[0] == 'Z' && suffix[1] == '\0'))
      throw std::runtime_error("Invalid ISO8601 timestamp '" + iso_string +
                               "': non-UTC timezone or extra characters");

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
      ParseISO8601(boost::json::value_to<std::string>(it->value()), false);
  }
  
  if (auto it = notam_obj.find("effectiveEnd"); it != notam_obj.end()) {
    notam.end_time =
      ParseISO8601(boost::json::value_to<std::string>(it->value()), true);
  }
  
  // Parse altitude limits using airspace altitude parsing system
  if (auto it = notam_obj.find("lowerLimit"); it != notam_obj.end()) {
    const auto lower_str = boost::json::value_to<std::string>(it->value());
    if (!Strip(std::string_view{lower_str}).empty()) {
      notam.lower_altitude = ParseAltitudeString(lower_str);
      has_lower_limit = true;
    }
  }
  
  if (auto it = notam_obj.find("upperLimit"); it != notam_obj.end()) {
    const auto upper_str = boost::json::value_to<std::string>(it->value());
    if (!Strip(std::string_view{upper_str}).empty()) {
      notam.upper_altitude = ParseAltitudeString(upper_str);
      has_upper_limit = true;
    }
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
  auto it = notam_obj.find("minimumFL");
  if (it == notam_obj.end())
    it = notam_obj.find("minimumFl");
  if (it != notam_obj.end()) {
    notam.minimum_fl = boost::json::value_to<std::string>(it->value());
    if (!has_lower_limit)
      notam.lower_altitude = ParseFlightLevelLimit(notam.minimum_fl);
  }
  
  it = notam_obj.find("maximumFL");
  if (it == notam_obj.end())
    it = notam_obj.find("maximumFl");
  if (it != notam_obj.end()) {
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

static std::vector<NOTAM::NOTAMGeometry>
ParseNOTAMGeometries(const NOTAM &base_notam,
                     const boost::json::object &feature,
                     const boost::json::object &notam_obj)
{
  const auto apply_radius_if_present =
    [&notam_obj](NOTAM &notam) {
      auto it = notam_obj.find("radius");
      if (it == notam_obj.end())
        return;

      notam.geometry.type = NOTAM::NOTAMGeometry::Type::CIRCLE;

      try {
        double radius_nm = 0;
        const auto &value = it->value();
        if (value.is_string()) {
          const auto s = boost::json::value_to<std::string>(value);
          std::size_t pos = 0;
          radius_nm = std::stod(s, &pos);
          if (pos != s.size())
            throw std::invalid_argument("extra characters after radius");
        } else {
          radius_nm = value.to_number<double>();
        }

        if (!std::isfinite(radius_nm))
          throw std::invalid_argument("non-finite NOTAM radius");

        if (radius_nm <= 0)
          throw std::invalid_argument("non-positive NOTAM radius");

        const double radius_meters = radius_nm * NM_TO_METERS;
        if (!std::isfinite(radius_meters))
          throw std::invalid_argument("non-finite NOTAM radius in meters");

        notam.geometry.radius_meters = radius_meters;
      } catch (const std::exception &) {
        LogFmt("Failed to parse NOTAM radius for {}",
               notam.number.c_str());
        notam.geometry.radius_meters = DEFAULT_NOTAM_RADIUS_METERS;
      }
    };

  const auto parse_geometry =
    [&](const boost::json::object &geometry, auto &&self)
      -> std::vector<NOTAM::NOTAMGeometry> {
      const auto type_it = geometry.find("type");
      if (type_it == geometry.end() || !type_it->value().is_string())
        throw std::runtime_error("Geometry missing string type");

      const auto &geom_type = type_it->value().as_string();

      if (geom_type == "GeometryCollection"sv) {
        const auto geometries_it = geometry.find("geometries");
        if (geometries_it == geometry.end() || !geometries_it->value().is_array())
          throw std::runtime_error("GeometryCollection missing geometries");

        const auto &geometries = geometries_it->value().as_array();
        if (geometries.empty())
          throw std::runtime_error("GeometryCollection is empty");

        std::vector<NOTAM::NOTAMGeometry> parsed_geometries;
        for (const auto &entry : geometries) {
          if (!entry.is_object())
            continue;

          try {
            auto child_geometries = self(entry.as_object(), self);
            parsed_geometries.insert(parsed_geometries.end(),
                                     std::make_move_iterator(
                                       child_geometries.begin()),
                                     std::make_move_iterator(
                                       child_geometries.end()));
          } catch (const std::exception &) {
          }
        }

        if (parsed_geometries.empty())
          throw std::runtime_error("GeometryCollection has no usable geometry");

        return parsed_geometries;
      }

      NOTAM notam = base_notam;

      if (geom_type == "Point"sv) {
        const auto coords_it = geometry.find("coordinates");
        if (coords_it == geometry.end() || !coords_it->value().is_array())
          throw std::runtime_error("Point missing coordinates");

        const auto &coords = coords_it->value().as_array();
        if (coords.empty())
          throw std::runtime_error("Point has empty coordinates");

        notam.geometry.type = NOTAM::NOTAMGeometry::Type::POINT;
        notam.geometry.center =
          boost::json::value_to<NOTAMPoint>(coords_it->value());
        notam.geometry.radius_meters = 0.0;
        notam.geometry.polygon_points.clear();
        apply_radius_if_present(notam);
        return {std::move(notam.geometry)};
      }

      if (geom_type == "Polygon"sv) {
        const auto coords_it = geometry.find("coordinates");
        if (coords_it == geometry.end() || !coords_it->value().is_array())
          throw std::runtime_error("Polygon missing coordinates");

        const auto &coords = coords_it->value().as_array();
        if (coords.empty() || !coords.front().is_array())
          throw std::runtime_error("Polygon missing exterior ring");

        const auto &ring = coords.front().as_array();
        if (ring.empty())
          throw std::runtime_error("Polygon exterior ring is empty");

        notam.geometry.type = NOTAM::NOTAMGeometry::Type::POLYGON;
        notam.geometry.radius_meters = 0.0;
        notam.geometry.polygon_points.clear();
        notam.geometry.polygon_points.reserve(ring.size());

        for (const auto &coord : ring)
          notam.geometry.polygon_points.emplace_back(
            boost::json::value_to<NOTAMPoint>(coord));

        std::size_t center_count = notam.geometry.polygon_points.size();
        if (center_count > 1) {
          const auto &first = notam.geometry.polygon_points.front();
          const auto &last = notam.geometry.polygon_points.back();
          if (first.longitude == last.longitude &&
              first.latitude == last.latitude)
            --center_count;
        }

        if (center_count < 3)
          throw std::runtime_error(
            "Polygon exterior ring has fewer than three points");

        double sum_lon = 0;
        double sum_lat = 0;
        const double reference_lon =
          notam.geometry.polygon_points.front().longitude;
        for (std::size_t i = 0; i < center_count; ++i) {
          sum_lon += UnwrapLongitude(
            notam.geometry.polygon_points[i].longitude, reference_lon);
          sum_lat += notam.geometry.polygon_points[i].latitude;
        }

        const double inv_count = 1.0 / static_cast<double>(center_count);
        notam.geometry.center =
          NOTAMPoint{NormalizeLongitude(sum_lon * inv_count),
                     sum_lat * inv_count};
        const auto center = ToGeoPoint(notam.geometry.center);
        for (const auto &point : notam.geometry.polygon_points)
          notam.geometry.radius_meters =
            std::max(notam.geometry.radius_meters,
                     center.Distance(ToGeoPoint(point)));

        return {std::move(notam.geometry)};
      }

      throw std::runtime_error("Unsupported geometry type");
    };

  const auto geometry_it = feature.find("geometry");
  if (geometry_it == feature.end() || !geometry_it->value().is_object())
    throw std::runtime_error("Feature missing geometry object");

  return parse_geometry(geometry_it->value().as_object(), parse_geometry);
}

static std::vector<NOTAM>
ParseNOTAMFeature(const boost::json::object &feature)
{
  NOTAM notam;

  // Get the nested NOTAM data
  const auto &props = feature.at("properties").as_object();
  const auto &core_notam_data = props.at("coreNOTAMData").as_object();
  const auto &notam_obj = core_notam_data.at("notam").as_object();

  ParseNOTAMProperties(notam, notam_obj);
  const auto geometries = ParseNOTAMGeometries(notam, feature, notam_obj);

  std::vector<NOTAM> notams;
  notams.reserve(geometries.size());
  for (auto geometry : geometries) {
    auto copy = notam;
    copy.geometry = std::move(geometry);
    notams.emplace_back(std::move(copy));
  }

  return notams;
}

static std::vector<NOTAM>
ParseNOTAMItems(const boost::json::array &items)
{
  std::vector<NOTAM> notams;
  notams.reserve(items.size());
  std::string last_error;

  for (const auto &item_val : items) {
    try {
      if (!item_val.is_object())
        throw std::runtime_error("NOTAM item is not an object");

      const auto &feature = item_val.as_object();
      auto parsed_notams = ParseNOTAMFeature(feature);
      notams.insert(notams.end(),
                    std::make_move_iterator(parsed_notams.begin()),
                    std::make_move_iterator(parsed_notams.end()));
    } catch (const std::exception &e) {
      // Skip invalid NOTAMs but continue processing others
      last_error = e.what();
      LogDebug("NOTAM: Skipped invalid item: {}", e.what());
      continue;
    }
  }

  if (!items.empty() && notams.empty()) {
    if (!last_error.empty())
      throw std::runtime_error(fmt::format(
        "All NOTAM items failed to parse: {}", last_error));

    throw std::runtime_error("All NOTAM items failed to parse");
  }
  
  return notams;
}

static std::vector<std::string>
ParseRemovedIds(const boost::json::object &root)
{
  std::vector<std::string> removed_ids;

  if (auto it = root.find("removedIds"); it != root.end()) {
    if (!it->value().is_array())
      throw std::runtime_error(
        "Invalid NOTAM response: removedIds is not an array");

    const auto &removed = it->value().as_array();
    removed_ids.reserve(removed.size());
    for (const auto &entry : removed) {
      if (!entry.is_string())
        throw std::runtime_error(
          "Invalid NOTAM response: removedIds entry is not a string");

      removed_ids.emplace_back(
        boost::json::value_to<std::string>(entry));
    }
  }

  return removed_ids;
}

std::vector<NOTAM>
ParseNOTAMGeoJSON(const boost::json::value &json)
{
  const auto &root = json.as_object();

  // The API returns items array instead of features
  const auto &items = root.at("items").as_array();

  return ParseNOTAMItems(items);
}

NOTAMResponse
ParseNOTAMResponse(const boost::json::value &json)
{
  NOTAMResponse response;

  const auto &root = json.as_object();
  if (auto it = root.find("delta");
      it != root.end() && it->value().is_bool()) {
    response.is_delta = it->value().as_bool();
  }

  response.removed_ids = ParseRemovedIds(root);

  if (auto it = root.find("items"); it != root.end()) {
    if (!it->value().is_array())
      throw std::runtime_error(
        "Invalid NOTAM response: 'items' is present but is not an array");
    response.notams = ParseNOTAMItems(it->value().as_array());
  } else if (!response.is_delta) {
    throw std::runtime_error("Invalid NOTAM response: missing items array");
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
  if (!location.IsValid()) {
    LogFmt("NOTAM: Request location is invalid");
    throw std::runtime_error("NOTAM request location is invalid");
  }

  const char *const base_url = settings.api_base_url.c_str();
  if (base_url[0] == '\0') {
    LogFmt("NOTAM: API base URL is empty");
    throw std::runtime_error("NOTAM API base URL is empty");
  }
  const double raw_radius_nm = (settings.radius_km * 1000.0) / NM_TO_METERS;
  const double max_radius_nm =
    (MAX_NOTAM_REQUEST_RADIUS_KM * 1000.0) / NM_TO_METERS;
  const double radius_nm = std::clamp(raw_radius_nm, 0.0, max_radius_nm);
  if (raw_radius_nm > max_radius_nm)
    LogFmt("NOTAM: Requested radius {:.1f} NM clamped to {:.1f} NM",
           raw_radius_nm, max_radius_nm);
  const char separator = std::strchr(base_url, '?') != nullptr ? '&' : '?';
  const auto url = fmt::format(
    "{}{}locationLongitude={}&locationLatitude={}&locationRadius={:.2f}",
    base_url, separator,
    location.longitude.Degrees(),
    location.latitude.Degrees(),
    radius_nm);

  LogDebug("NOTAM: Fetching from API with radius {:.1f} NM",
           radius_nm);

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

  LogFmt("NOTAM: Response status={}, body={} bytes",
         http_response.status,
         static_cast<unsigned long>(http_response.body.size()));

  if (http_response.status != 200) {
    LogFmt("NOTAM: HTTP {} with {} response bytes",
           http_response.status,
           static_cast<unsigned long>(http_response.body.size()));
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
