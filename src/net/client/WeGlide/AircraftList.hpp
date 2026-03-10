// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The XCSoar Project

#pragma once

#include "util/StaticString.hxx"

#ifdef HAVE_HTTP
#include "co/Task.hxx"
#endif

#include <cstdint>
#include <vector>

struct WeGlideSettings;
class ProgressListener;
class CurlGlobal;

namespace WeGlide {

struct AircraftType {
  uint32_t id = 0;
  StaticString<96> name;
};

#ifdef HAVE_HTTP

std::vector<AircraftType>
LoadAircraftListCache();

bool
LookupAircraftTypeName(unsigned aircraft_id,
                       StaticString<96> &name);

Co::Task<std::vector<AircraftType>>
DownloadAircraftList(::CurlGlobal &curl, const WeGlideSettings &settings,
                     ProgressListener &progress);

#else

inline std::vector<AircraftType>
LoadAircraftListCache()
{
  return {};
}

inline bool
LookupAircraftTypeName([[maybe_unused]] unsigned aircraft_id,
                       [[maybe_unused]] StaticString<96> &name)
{
  return false;
}

#endif

} // namespace WeGlide
