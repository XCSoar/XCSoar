// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The XCSoar Project

#include "NOTAMCache.hpp"
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
  } catch (...) {
    return false;
  }
}

CacheMetadata
NOTAMCache::ExtractMetadata(const boost::json::object &obj) noexcept
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

  meta.valid = has_timestamp && has_location && has_radius;
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
                       const std::function<bool()> &validate_commit)
{
  if (!location.IsValid())
    throw std::invalid_argument("invalid NOTAM cache location");

  boost::json::object wrapper;
  wrapper["xcsoar_timestamp"] = static_cast<std::int64_t>(std::time(nullptr));
  wrapper["xcsoar_location_lat"] = location.latitude.Degrees();
  wrapper["xcsoar_location_lon"] = location.longitude.Degrees();
  wrapper["xcsoar_radius_km"] = radius_km;
  wrapper["xcsoar_refresh_interval_min"] = refresh_interval_min;
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
