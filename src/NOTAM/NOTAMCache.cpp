// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The XCSoar Project

#include "NOTAMCache.hpp"
#include "Client.hpp"
#include "Delta.hpp"
#include "NOTAM.hpp"
#include "Settings.hpp"
#include "io/FileOutputStream.hxx"
#include "io/FileReader.hxx"
#include "io/StringOutputStream.hxx"
#include "json/Parse.hxx"
#include "json/Serialize.hxx"
#include "system/FileUtil.hpp"
#include "LocalPath.hpp"
#include "LogFile.hpp"

#include <boost/json.hpp>

#include <cmath>
#include <cstring>
#include <limits>
#include <stdexcept>

static bool
GetJsonNumber(const boost::json::value &value, double &out) noexcept
{
  if (value.is_double()) {
    out = value.as_double();
    return true;
  }
  if (value.is_int64()) {
    out = static_cast<double>(value.as_int64());
    return true;
  }
  if (value.is_uint64()) {
    out = static_cast<double>(value.as_uint64());
    return true;
  }
  return false;
}

AllocatedPath
NOTAMCache::GetFilePath()
{
  return LocalPath("notams.json");
}

bool
NOTAMCache::LoadJsonValue(const AllocatedPath &file_path,
                          boost::json::value &root)
{
  try {
    FileReader file(Path(file_path.c_str()));
    root = Json::Parse(file);
    return true;
  } catch (const std::exception &e) {
    const auto path_utf8 = file_path.ToUTF8();
    LogFmt("NOTAM: Failed to load cache {}: {}",
           path_utf8.c_str(), e.what());
    return false;
  } catch (...) {
    const auto path_utf8 = file_path.ToUTF8();
    LogFmt("NOTAM: Failed to load cache {}", path_utf8.c_str());
    return false;
  }
}

CacheMetadata
NOTAMCache::ExtractMetadata(const boost::json::object &obj)
{
  CacheMetadata meta;
  constexpr double min_time_t =
    static_cast<double>(std::numeric_limits<std::time_t>::min());
  constexpr double max_time_t =
    static_cast<double>(std::numeric_limits<std::time_t>::max());
  constexpr double max_unsigned =
    static_cast<double>(std::numeric_limits<unsigned>::max());

  double timestamp = 0;
  bool has_timestamp = false;
  if (auto it = obj.find("xcsoar_timestamp");
      it != obj.end() && GetJsonNumber(it->value(), timestamp)) {
    if (std::isfinite(timestamp) &&
        timestamp >= min_time_t && timestamp <= max_time_t) {
      meta.timestamp = static_cast<std::time_t>(timestamp);
      has_timestamp = true;
    }
  }

  double lat = 0, lon = 0;
  bool has_location = false;
  if (auto it_lat = obj.find("xcsoar_location_lat");
      it_lat != obj.end() && GetJsonNumber(it_lat->value(), lat)) {
    if (auto it_lon = obj.find("xcsoar_location_lon");
        it_lon != obj.end() && GetJsonNumber(it_lon->value(), lon)) {
      if (std::isfinite(lat) && lat >= -90 && lat <= 90 &&
          std::isfinite(lon) && lon >= -180 && lon <= 180) {
        meta.location = GeoPoint(Angle::Degrees(lon), Angle::Degrees(lat));
        has_location = true;
      }
    }
  }

  double radius = 0;
  bool has_radius = false;
  if (auto it_r = obj.find("xcsoar_radius_km");
      it_r != obj.end() && GetJsonNumber(it_r->value(), radius)) {
    if (std::isfinite(radius) && radius >= 0 && radius <= max_unsigned) {
      meta.radius_km = static_cast<unsigned>(radius);
      has_radius = true;
    }
  }

  bool has_api_base_url = false;
  if (auto it = obj.find("api_base_url");
      it != obj.end() && it->value().is_string()) {
    const auto &api_base_url = it->value().as_string();
    if (!api_base_url.empty()) {
      meta.api_base_url = api_base_url.c_str();
      has_api_base_url = true;
    }
  }

  meta.valid = has_timestamp && has_location && has_radius &&
    has_api_base_url;
  return meta;
}

std::optional<CacheMetadata>
NOTAMCache::LoadMetadataFromFile(const AllocatedPath &file_path)
{
  boost::json::value root;
  if (!LoadJsonValue(file_path, root) || !root.is_object())
    return std::nullopt;

  CacheMetadata meta = ExtractMetadata(root.as_object());
  return meta.valid ? std::optional<CacheMetadata>{meta} : std::nullopt;
}

void
NOTAMCache::SaveToFile(const AllocatedPath &file_path,
                       const boost::json::value &api_response,
                       const GeoPoint &location,
                       const unsigned radius_km,
                       const unsigned refresh_interval_min,
                       const char *const api_base_url,
                       const std::function<bool()> &validate_commit)
{
  if (!location.IsValid())
    throw std::invalid_argument("invalid NOTAM cache location");

  if (api_base_url == nullptr || api_base_url[0] == '\0')
    throw std::invalid_argument("invalid NOTAM API base URL");

  boost::json::object wrapper;
  wrapper["xcsoar_timestamp"] = static_cast<std::int64_t>(std::time(nullptr));
  wrapper["xcsoar_location_lat"] = location.latitude.Degrees();
  wrapper["xcsoar_location_lon"] = location.longitude.Degrees();
  wrapper["xcsoar_radius_km"] = radius_km;
  wrapper["xcsoar_refresh_interval_min"] = refresh_interval_min;
  wrapper["api_base_url"] = api_base_url;
  wrapper["api"] = api_response;

  StringOutputStream sos;
  Json::Serialize(sos, boost::json::value(std::move(wrapper)));
  const std::string payload = std::move(sos).GetValue();

  FileOutputStream file(Path(file_path.c_str()));
  file.Write(std::as_bytes(std::span{payload}));

  if (validate_commit != nullptr && !validate_commit())
    throw std::runtime_error("stale NOTAM cache generation before commit");

  file.Commit();

  LogFmt("NOTAM: Saved {} bytes of NOTAM cache",
         static_cast<unsigned>(payload.size()));
}

void
NOTAMCache::InvalidateFile(const AllocatedPath &file_path)
{
  if (File::Exists(file_path))
    File::Delete(file_path);
}

bool
NOTAMCache::LoadBundle(const AllocatedPath &file_path, NOTAMCacheBundle &bundle)
{
  try {
    boost::json::value root;
    if (!LoadJsonValue(file_path, root) || !root.is_object()) {
      LogFmt("NOTAM: LoadBundle failed to load or parse file");
      return false;
    }

    const auto &obj = root.as_object();
    bundle.meta = ExtractMetadata(obj);

    const auto it = obj.find("api");
    if (it == obj.end()) {
      LogFmt("NOTAM: LoadBundle missing api payload");
      return false;
    }

    bundle.api = it->value();
    if (!NOTAMDelta::IsApiResponseValid(bundle.api)) {
      LogFmt("NOTAM: LoadBundle invalid api payload");
      return false;
    }

    try {
      bundle.notams = NOTAMClient::ParseNOTAMGeoJSON(bundle.api);
    } catch (const std::exception &e) {
      LogFmt("NOTAM: LoadBundle parse error: {}", e.what());
      return false;
    }

    LogFmt("NOTAM: LoadBundle parsed {} NOTAMs",
           static_cast<unsigned>(bundle.notams.size()));
    return true;
  } catch (const std::exception &e) {
    LogFmt("NOTAM: LoadBundle exception: {}", e.what());
    return false;
  }
}

bool
NOTAMCache::IsExpired(const CacheMetadata &meta,
                      const NOTAMSettings &settings,
                      const GeoPoint current_location) noexcept
{
  try {
    if (meta.timestamp <= 0)
      return true;

    if (!meta.location.IsValid())
      return true;

    if (meta.radius_km == 0)
      return true;

    const std::time_t current_time = std::time(nullptr);
    const std::time_t max_age_seconds =
      static_cast<std::time_t>(settings.refresh_interval_min) * 60;

    if (max_age_seconds > 0 &&
        (current_time - meta.timestamp) > max_age_seconds)
      return true;

    if (meta.radius_km != settings.radius_km)
      return true;

    if (meta.api_base_url != settings.api_base_url.c_str())
      return true;

    if (current_location.IsValid()) {
      const double distance_m = current_location.Distance(meta.location);
      const double radius_m = settings.radius_km * 1000.0;
      if (distance_m > radius_m)
        return true;
    }

    return false;
  } catch (const std::exception &) {
    return true;
  }
}
