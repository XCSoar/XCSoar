// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The XCSoar Project

#include "Sync.hpp"
#include "Delta.hpp"
#include "NOTAMCache.hpp"
#include "LogFile.hpp"
#include "Operation/ProgressListener.hpp"
#include "lib/curl/Global.hxx"

#include <stdexcept>

#ifdef HAVE_HTTP

namespace NOTAMSync {

Co::Task<Result>
Fetch(CurlGlobal &curl,
      const NOTAMSettings &settings,
      const GeoPoint location,
      const bool force_network,
      const bool cache_expired,
      const AllocatedPath &cache_path,
      ProgressListener &progress,
      const CachedState &state)
{
  Result result;
  result.known_location = location;
  result.known_radius_km = settings.radius_km;

  LogFmt("NOTAM: Starting sync");

  if (!force_network && !cache_expired) {
    LogFmt("NOTAM: Cache is still valid, attempting to load from cache");
    NOTAMCache::NOTAMCacheBundle bundle;
    if (NOTAMCache::LoadBundle(cache_path, bundle)) {
      LogFmt("NOTAM: Successfully loaded {} NOTAMs from cache",
             static_cast<unsigned>(bundle.notams.size()));
      result.outcome = Outcome::DiskCache;
      result.notams = std::move(bundle.notams);
      result.document = std::move(bundle.api);
      result.known_location = bundle.meta.location.IsValid()
        ? bundle.meta.location
        : location;
      result.known_radius_km = bundle.meta.radius_km > 0
        ? bundle.meta.radius_km
        : settings.radius_km;
      co_return result;
    }

    LogFmt("NOTAM: Failed to load from cache file, will fetch fresh data");
  } else {
    LogFmt("NOTAM: Cache is expired or doesn't exist, fetching fresh data");
  }

  LogFmt("NOTAM: Starting API fetch for radius {} km", settings.radius_km);

  NOTAMClient::KnownMap known_copy;
  if (NOTAMDelta::CanUseDelta(state.known, state.cached_api,
                              state.cached_api_valid,
                              state.known_location, state.known_radius_km,
                              location, settings.radius_km))
    known_copy = state.known;

  if (!known_copy.empty()) {
    try {
      LogFmt("NOTAM: Attempting delta fetch with {} known IDs",
             static_cast<unsigned>(known_copy.size()));
      auto delta_response =
        co_await NOTAMClient::FetchNOTAMsResponse(curl, settings, location,
                                                  progress, &known_copy);

      if (delta_response.is_delta) {
        if (!delta_response.document.is_object()) {
          LogFmt("NOTAM: Delta response missing JSON object, falling back");
        } else if (state.cached_api_valid) {
          auto cached_api = state.cached_api;
          auto current_notams = state.current_notams;

          if (NOTAMDelta::ApplyDeltaToApi(cached_api, delta_response.document,
                                          delta_response.removed_ids)) {
            NOTAMDelta::ApplyDeltaUpdates(current_notams,
                                          delta_response.notams,
                                          delta_response.removed_ids);

            result.outcome = Outcome::Delta;
            result.notams = std::move(current_notams);
            result.document = std::move(cached_api);
            result.known_location = location;
            result.known_radius_km = settings.radius_km;
            result.delta_updates =
              static_cast<unsigned>(delta_response.notams.size());
            result.delta_removed =
              static_cast<unsigned>(delta_response.removed_ids.size());

            LogFmt("NOTAM: Delta applied (updates={}, removed={}, total={})",
                   result.delta_updates, result.delta_removed,
                   static_cast<unsigned>(result.notams.size()));

            co_return result;
          }

          LogFmt("NOTAM: Delta merge failed, falling back to full fetch");
        }
      } else if (NOTAMDelta::IsApiResponseValid(delta_response.document)) {
        result.outcome = Outcome::Full;
        result.notams = std::move(delta_response.notams);
        result.document = std::move(delta_response.document);

        LogFmt("NOTAM: Full response accepted (total={})",
               static_cast<unsigned>(result.notams.size()));

        co_return result;
      } else {
        LogFmt("NOTAM: Delta response missing full API payload");
      }

      LogFmt("NOTAM: Delta not applied, falling back to full fetch");
    } catch (const std::exception &e) {
      LogFmt("NOTAM: Delta fetch failed: {}", e.what());
    }
  }

  LogFmt("NOTAM: Performing full fetch");
  auto full_response =
    co_await NOTAMClient::FetchNOTAMsResponse(curl, settings, location,
                                              progress, nullptr);
  if (!NOTAMDelta::IsApiResponseValid(full_response.document)) {
    LogFmt("NOTAM: Rejecting invalid API payload");
    throw std::runtime_error("Invalid NOTAM API payload");
  }

  result.outcome = Outcome::Full;
  result.notams = std::move(full_response.notams);
  result.document = std::move(full_response.document);

  LogFmt("NOTAM: Parsed {} NOTAMs from response",
         static_cast<unsigned>(result.notams.size()));

  co_return result;
}

} // namespace NOTAMSync

#endif // HAVE_HTTP
