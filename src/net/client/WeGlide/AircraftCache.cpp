// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The XCSoar Project

#include "AircraftCache.hpp"

#include "LocalPath.hpp"
#include "io/FileLineReader.hpp"
#include "io/FileOutputStream.hxx"
#include "system/FileUtil.hpp"
#include "json/Serialize.hxx"

#include <boost/json.hpp>

#include <string>

namespace WeGlide {

static constexpr auto kAircraftCacheFile = "weglide-aircraft-v1.json";

static AllocatedPath
GetAircraftCachePath() noexcept
{
  return AllocatedPath::Build(GetCachePath(), kAircraftCacheFile);
}

bool
LoadAircraftListCacheValue(boost::json::value &value)
{
  std::string json;

  try {
    FileLineReaderA reader(GetAircraftCachePath());
    const char *line;
    while ((line = reader.ReadLine()) != nullptr) {
      json += line;
      json += '\n';
    }
  } catch (...) {
    return false;
  }

  if (json.empty())
    return false;

  try {
    value = boost::json::parse(json);
    return true;
  } catch (...) {
    return false;
  }
}

void
StoreAircraftListCacheValue(const boost::json::value &value)
{
  Directory::Create(GetCachePath());
  FileOutputStream file(GetAircraftCachePath());
  Json::Serialize(file, value);
  file.Commit();
}

} // namespace WeGlide
