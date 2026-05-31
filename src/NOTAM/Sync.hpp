// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The XCSoar Project

#pragma once

#include "NOTAM.hpp"
#include "Settings.hpp"
#include "Client.hpp"
#include "system/Path.hpp"
#include "Geo/GeoPoint.hpp"
#include "co/Task.hxx"

#include <boost/json/fwd.hpp>
#include <vector>

class CurlGlobal;
class ProgressListener;

namespace NOTAMSync {

struct CachedState {
  std::vector<struct NOTAM> current_notams;
  NOTAMClient::KnownMap known;
  boost::json::value cached_api;
  bool cached_api_valid = false;
  GeoPoint known_location = GeoPoint::Invalid();
  unsigned known_radius_km = 0;
};

enum class Outcome {
  DiskCache,
  Delta,
  Full,
};

struct Result {
  Outcome outcome = Outcome::Full;
  std::vector<struct NOTAM> notams;
  boost::json::value document;
  GeoPoint known_location = GeoPoint::Invalid();
  unsigned known_radius_km = 0;
  unsigned delta_updates = 0;
  unsigned delta_removed = 0;
};

[[nodiscard]]
Co::Task<Result>
Fetch(CurlGlobal &curl,
      const NOTAMSettings &settings,
      GeoPoint location,
      bool force_network,
      bool cache_expired,
      const AllocatedPath &cache_path,
      ProgressListener &progress,
      const CachedState &state);

} // namespace NOTAMSync
