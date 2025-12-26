// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The XCSoar Project

#pragma once

#include "Geo/GeoPoint.hpp"
#include "system/Path.hpp"

#include <boost/json/fwd.hpp>
#include <ctime>
#include <functional>
#include <optional>
#include <string>

struct CacheMetadata {
  std::time_t timestamp = 0;
  GeoPoint location = GeoPoint::Invalid();
  unsigned radius_km = 0;
  std::string api_base_url;
  bool valid = false;
};

namespace NOTAMCache {

/** Returns the path to the NOTAM cache file. */
AllocatedPath GetFilePath();

/**
 * Read and parse the NOTAM cache file into a JSON value using
 * Json::Parse(FileReader &) (stream_parser, no full-file buffer).
 * @return false on any I/O or parse error.
 */
[[nodiscard]]
bool LoadJsonValue(const AllocatedPath &file_path,
                   boost::json::value &root);

/** Extract cache metadata fields from the top-level JSON object. */
CacheMetadata ExtractMetadata(const boost::json::object &obj);

/** Load only the metadata fields from the cache file. */
[[nodiscard]] std::optional<CacheMetadata>
LoadMetadataFromFile(const AllocatedPath &file_path);

/**
 * Write api_response wrapped with XCSoar metadata to the cache file
 * atomically (FileOutputStream::Commit).  Throws on I/O error.
 */
void SaveToFile(const AllocatedPath &file_path,
                const boost::json::value &api_response,
                const GeoPoint &location,
                unsigned radius_km,
                unsigned refresh_interval_min,
                const char *api_base_url,
                const std::function<bool()> &validate_commit = {});

/** Delete the cache file if it exists. */
void InvalidateFile(const AllocatedPath &file_path);

} // namespace NOTAMCache
