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
#include "Formatter/TimeFormatter.hpp"
#include "Engine/Airspace/AirspaceAltitude.hpp"
#include "json/Geo.hpp"
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
#include <optional>
#include <stdexcept>
#include <string_view>

using std::string_view_literals::operator""sv;

namespace {
constexpr double NM_TO_METERS = 1852.0;

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

static std::optional<AirspaceAltitude>
ParseAltitudeString(const std::string &alt_str)
{
  if (alt_str.empty()) {
    AirspaceAltitude altitude{};
    altitude.reference = AltitudeReference::MSL;
    altitude.altitude = NOTAMAltitude::INVALID_ALTITUDE;
    altitude.flight_level = 0;
    altitude.altitude_above_terrain = 0;
    return altitude;
  }

  ParseAirspaceAltitudeOptions options;
  options.strict_unknown_tokens = true;
  options.accept_amsl = true;
  options.unlimited_ceiling_m = Units::ToSysUnit(50000, Unit::FEET);

  StringParser<> input{alt_str.c_str()};
  const auto altitude = ParseAirspaceAltitude(input, options);
  if (!altitude) {
    LogFmt("NOTAM: Unexpected altitude token near '{}' in '{}'",
           input.c_str(), alt_str.c_str());
    return std::nullopt;
  }

  return altitude;
}

static std::optional<AirspaceAltitude>
ParseFlightLevelLimit(const std::string &fl_value)
{
  const auto trimmed = Strip(std::string_view{fl_value});
  if (trimmed.empty())
    return std::nullopt;

  if (trimmed == "000"sv)
    return ParseAltitudeString("GND");

  if (trimmed == "999"sv)
    return ParseAltitudeString("UNL");

  std::string fl;
  fl.reserve(2 + trimmed.size());
  fl.append("FL");
  fl.append(trimmed);
  return ParseAltitudeString(fl);
}

} // namespace

namespace NOTAMClient {

static std::chrono::system_clock::time_point
ParseNOTAMTime(const std::string &iso_string, const bool allow_permanent)
{
  if (iso_string == "PERM") {
    if (!allow_permanent)
      throw std::runtime_error("Invalid ISO8601 timestamp '" + iso_string +
                               "': PERM is only valid as an end time");

    return NOTAMTime::PermanentEndTime();
  }

  return ParseISO8601Utc(iso_string);
}

static void
ParseNOTAMProperties(NOTAM &notam, const boost::json::object &notam_obj)
{
  bool has_lower_limit = false;
  bool has_upper_limit = false;

  notam.lower_altitude = ParseAltitudeString("").value();
  notam.upper_altitude = ParseAltitudeString("").value();

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
      ParseNOTAMTime(boost::json::value_to<std::string>(it->value()), false);
  }
  
  if (auto it = notam_obj.find("effectiveEnd"); it != notam_obj.end()) {
    notam.end_time =
      ParseNOTAMTime(boost::json::value_to<std::string>(it->value()), true);
  }
  
  // Parse altitude limits using airspace altitude parsing system
  if (auto it = notam_obj.find("lowerLimit"); it != notam_obj.end()) {
    const auto lower_str = boost::json::value_to<std::string>(it->value());
    if (!Strip(std::string_view{lower_str}).empty()) {
      if (const auto altitude = ParseAltitudeString(lower_str)) {
        notam.lower_altitude = *altitude;
        has_lower_limit = true;
      }
    }
  }
  
  if (auto it = notam_obj.find("upperLimit"); it != notam_obj.end()) {
    const auto upper_str = boost::json::value_to<std::string>(it->value());
    if (!Strip(std::string_view{upper_str}).empty()) {
      if (const auto altitude = ParseAltitudeString(upper_str)) {
        notam.upper_altitude = *altitude;
        has_upper_limit = true;
      }
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
    if (!has_lower_limit) {
      const auto altitude = ParseFlightLevelLimit(notam.minimum_fl);
      if (!altitude)
        throw std::runtime_error("Malformed minimumFL '" + notam.minimum_fl +
                                 "'");

      notam.lower_altitude = *altitude;
    }
  }
  
  it = notam_obj.find("maximumFL");
  if (it == notam_obj.end())
    it = notam_obj.find("maximumFl");
  if (it != notam_obj.end()) {
    notam.maximum_fl = boost::json::value_to<std::string>(it->value());
    if (!has_upper_limit) {
      const auto altitude = ParseFlightLevelLimit(notam.maximum_fl);
      if (!altitude)
        throw std::runtime_error("Malformed maximumFL '" + notam.maximum_fl +
                                 "'");

      notam.upper_altitude = *altitude;
    }
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
ParseNOTAMGeometries(const struct NOTAM &base_notam,
                     const boost::json::object &feature,
                     const boost::json::object &notam_obj)
{
  const auto apply_radius_if_present =
    [&notam_obj](struct NOTAM &notam) {
      auto it = notam_obj.find("radius");
      if (it == notam_obj.end())
        return;

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
        notam.geometry.type = NOTAM::NOTAMGeometry::Type::CIRCLE;
      } catch (const std::exception &) {
        LogFmt("Failed to parse NOTAM radius for {}",
               notam.number.c_str());
        notam.geometry.radius_meters = 0.;
      }
    };

  const auto parse_geometry =
    [&](const boost::json::object &geometry, auto &&self)
      -> std::vector<NOTAM::NOTAMGeometry> {
      const auto type_it = geometry.find("type");
      if (type_it == geometry.end() || !type_it->value().is_string())
        throw std::runtime_error("Geometry missing string type");

      const std::string_view geom_type{type_it->value().as_string()};

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
          } catch (const std::exception &e) {
            LogDebug("NOTAM: GeometryCollection child skipped: {}",
                     e.what());
          }
        }

        if (parsed_geometries.empty())
          throw std::runtime_error("GeometryCollection has no usable geometry");

        return parsed_geometries;
      }

      struct NOTAM notam = base_notam;

      if (geom_type == "Point"sv) {
        const auto coords_it = geometry.find("coordinates");
        if (coords_it == geometry.end() || !coords_it->value().is_array())
          throw std::runtime_error("Point missing coordinates");

        const auto &coords = coords_it->value().as_array();
        if (coords.empty())
          throw std::runtime_error("Point has empty coordinates");

        notam.geometry.type = NOTAM::NOTAMGeometry::Type::POINT;
        notam.geometry.center =
          boost::json::value_to<GeoPoint>(coords_it->value());
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
        if (coords.size() > 1)
          throw std::runtime_error(
            "Polygon contains interior rings (holes) which are not supported");

        const auto &ring = coords.front().as_array();
        if (ring.empty())
          throw std::runtime_error("Polygon exterior ring is empty");

        notam.geometry.type = NOTAM::NOTAMGeometry::Type::POLYGON;
        notam.geometry.radius_meters = 0.0;
        notam.geometry.polygon_points.clear();
        notam.geometry.polygon_points.reserve(ring.size());

        for (const auto &coord : ring)
          notam.geometry.polygon_points.emplace_back(
            boost::json::value_to<GeoPoint>(coord));

        std::size_t center_count = notam.geometry.polygon_points.size();
        if (center_count > 1) {
          const auto &first = notam.geometry.polygon_points.front();
          const auto &last = notam.geometry.polygon_points.back();
          if (first.longitude.Degrees() == last.longitude.Degrees() &&
              first.latitude.Degrees() == last.latitude.Degrees())
            --center_count;
        }

        if (center_count < 3)
          throw std::runtime_error(
            "Polygon exterior ring has fewer than three points");

        double sum_lon = 0;
        double sum_lat = 0;
        const double reference_lon =
          notam.geometry.polygon_points.front().longitude.Degrees();
        for (std::size_t i = 0; i < center_count; ++i) {
          sum_lon += UnwrapLongitude(
            notam.geometry.polygon_points[i].longitude.Degrees(),
            reference_lon);
          sum_lat += notam.geometry.polygon_points[i].latitude.Degrees();
        }

        const double inv_count = 1.0 / static_cast<double>(center_count);
        notam.geometry.center = GeoPoint{
          Angle::Degrees(NormalizeLongitude(sum_lon * inv_count)),
          Angle::Degrees(sum_lat * inv_count)
        };
        if (!notam.geometry.center.Check())
          throw std::runtime_error("Invalid polygon centroid");

        for (const auto &point : notam.geometry.polygon_points)
          notam.geometry.radius_meters =
            std::max(notam.geometry.radius_meters,
                     notam.geometry.center.Distance(point));

        return {std::move(notam.geometry)};
      }

      throw std::runtime_error("Unsupported geometry type");
    };

  const auto geometry_it = feature.find("geometry");
  if (geometry_it == feature.end() || !geometry_it->value().is_object())
    throw std::runtime_error("Feature missing geometry object");

  return parse_geometry(geometry_it->value().as_object(), parse_geometry);
}

static std::vector<struct NOTAM>
ParseNOTAMFeature(const boost::json::object &feature)
{
  struct NOTAM notam;

  // Get the nested NOTAM data
  const auto &props = feature.at("properties").as_object();
  const auto &core_notam_data = props.at("coreNOTAMData").as_object();
  const auto &notam_obj = core_notam_data.at("notam").as_object();

  ParseNOTAMProperties(notam, notam_obj);
  if (notam.id.empty() || notam.last_updated.empty())
    throw std::runtime_error("NOTAM missing id or lastUpdated");
  auto geometries = ParseNOTAMGeometries(notam, feature, notam_obj);

  std::vector<struct NOTAM> notams;
  notams.reserve(geometries.size());
  for (auto &geometry : geometries) {
    auto copy = notam;
    copy.geometry = std::move(geometry);
    notams.emplace_back(std::move(copy));
  }

  return notams;
}

static std::vector<struct NOTAM>
ParseNOTAMItems(const boost::json::array &items)
{
  std::vector<struct NOTAM> notams;
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

std::vector<struct NOTAM>
ParseNOTAMGeoJSON(const boost::json::value &json)
{
  const auto &root = json.as_object();

  // The API returns items array instead of features
  const auto &items = root.at("items").as_array();

  return ParseNOTAMItems(items);
}

NOTAMResponse
ParseNOTAMResponse(boost::json::value json)
{
  NOTAMResponse response;
  response.document = std::move(json);

  const auto &root = response.document.as_object();
  if (auto it = root.find("delta"); it != root.end()) {
    if (!it->value().is_bool())
      throw std::runtime_error(
        "Invalid NOTAM response: delta is not a boolean");

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

std::vector<struct NOTAM>
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
  const std::string_view base_url_view{base_url};
  const char *const separator =
    base_url_view.ends_with('?') || base_url_view.ends_with('&') ? "" :
    base_url_view.find('?') != std::string_view::npos ? "&" : "?";
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

  if (http_response.status != 200) {
    LogFmt("NOTAM: HTTP {} with {} response bytes",
           http_response.status,
           static_cast<unsigned long>(http_response.body.size()));
    throw std::runtime_error(
      fmt::format("Failed to fetch NOTAMs: HTTP {}", http_response.status));
  }

  LogDebug("NOTAM: Response status={}, body={} bytes",
           http_response.status,
           static_cast<unsigned long>(http_response.body.size()));

  co_return ParseNOTAMResponse(boost::json::parse(http_response.body));
}

} // namespace NOTAMClient

#endif // HAVE_HTTP
