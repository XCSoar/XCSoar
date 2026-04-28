// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The XCSoar Project

#include "NOTAMGlue.hpp"
#include "NOTAMCache.hpp"
#include "NOTAM.hpp"
#include "Converter.hpp"
#include "Client.hpp"
#include "Filter.hpp"
#include "Components.hpp"
#include "DataComponents.hpp"
#include "Message.hpp"
#include "Airspace/AirspaceGlue.hpp"
#include "Airspace/Airspaces.hpp"
#include "Airspace/AirspaceClass.hpp"
#include "Engine/Airspace/AbstractAirspace.hpp"
#include "Protection.hpp"
#include "Operation/Operation.hpp"
#include "Operation/ProgressListener.hpp"
#include "system/Path.hpp"
#include "LogFile.hpp"
#include "util/StaticString.hxx"
#include "util/SpanCast.hxx"
#include "thread/Mutex.hxx"
#include "co/Task.hxx"
#include "co/InvokeTask.hxx"
#include "lib/curl/Global.hxx"
#include "time/Convert.hxx"
#include "Language/Language.hpp"

#include <optional>
#include <algorithm>
#include <cstring>
#include <stdexcept>
#include <string_view>
#include <unordered_map>
#include <unordered_set>

#include <boost/json.hpp>

// Use full struct name to avoid collision with AirspaceClass::NOTAM enum
using NOTAMStruct = struct NOTAM;

#ifdef HAVE_HTTP

static constexpr std::time_t MIN_NOTAM_REFRESH_ATTEMPT_INTERVAL_SECONDS =
    120;

// Private implementation to avoid template issues
struct NOTAMImpl {
  std::vector<NOTAMStruct> current_notams;
  NOTAMClient::KnownMap known;
  GeoPoint known_location = GeoPoint::Invalid();
  unsigned known_radius_km = 0;
  boost::json::value cached_api;
  bool cached_api_valid = false;
  CacheMetadata last_update;
  bool last_update_cached = false;
};

static NOTAMClient::KnownMap
BuildKnownMap(const std::vector<NOTAMStruct> &notams)
{
  NOTAMClient::KnownMap known;
  known.reserve(notams.size());
  for (const auto &notam : notams) {
    if (!notam.id.empty() && !notam.last_updated.empty())
      known.emplace(notam.id, notam.last_updated);
  }
  return known;
}

static bool
IsApiResponseValid(const boost::json::value &value)
{
  if (!value.is_object())
    return false;

  const auto &obj = value.as_object();
  auto it = obj.find("items");
  return it != obj.end() && it->value().is_array();
}

static void
PopulateImplFromCache(NOTAMImpl &impl,
                      std::vector<NOTAMStruct> &&cached_notams,
                      const GeoPoint &known_location,
                      unsigned known_radius_km,
                      boost::json::value cached_api)
{
  const bool cached_api_valid = IsApiResponseValid(cached_api);
  impl.current_notams = std::move(cached_notams);
  impl.known = BuildKnownMap(impl.current_notams);
  impl.known_location = known_location;
  impl.known_radius_km = known_radius_km;
  if (cached_api_valid)
    impl.cached_api = std::move(cached_api);
  else
    impl.cached_api = boost::json::value();
  impl.cached_api_valid = cached_api_valid;
  impl.last_update = CacheMetadata{};
  impl.last_update_cached = false;
}

static void
AddLoadedNOTAMsMessage(const unsigned total)
{
  StaticString<100> msg;
  if (total == 1)
    msg = _("Loaded 1 NOTAM");
  else
    msg.Format(_("Loaded %u NOTAMs"), total);

  Message::AddMessage(msg.c_str());
}

static bool
CanUseDelta(const NOTAMImpl &impl, const GeoPoint &location,
            unsigned radius_km)
{
  if (!location.IsValid())
    return false;

  if (!impl.cached_api_valid)
    return false;

  if (impl.known.empty() || !impl.known_location.IsValid())
    return false;

  if (impl.known_radius_km != radius_km)
    return false;

  const double radius_m = radius_km * 1000.0;
  const double distance_m = location.Distance(impl.known_location);
  return distance_m <= (radius_m * 0.5);
}

static void
ApplyDeltaUpdates(std::vector<NOTAMStruct> &current,
                  const std::vector<NOTAMStruct> &updates,
                  const std::vector<std::string> &removed_ids)
{
  if (!removed_ids.empty()) {
    std::unordered_set<std::string> removed(removed_ids.begin(),
                                            removed_ids.end());
    current.erase(std::remove_if(current.begin(), current.end(),
                                 [&removed](const NOTAMStruct &notam) {
                                   return !notam.id.empty() &&
                                     removed.contains(notam.id);
                                 }),
                  current.end());
  }

  if (!updates.empty()) {
    std::unordered_map<std::string, size_t> index;
    index.reserve(current.size());
    for (size_t i = 0; i < current.size(); ++i) {
      if (!current[i].id.empty())
        index.emplace(current[i].id, i);
    }

    for (const auto &update : updates) {
      if (update.id.empty())
        continue;

      auto it = index.find(update.id);
      if (it != index.end()) {
        current[it->second] = update;
      } else {
        index.emplace(update.id, current.size());
        current.push_back(update);
      }
    }
  }
}

static bool
GetNotamIdFromItem(const boost::json::value &item, std::string &id)
{
  if (!item.is_object())
    return false;

  const auto &obj = item.as_object();
  auto it_props = obj.find("properties");
  if (it_props == obj.end() || !it_props->value().is_object())
    return false;

  const auto &props = it_props->value().as_object();
  auto it_core = props.find("coreNOTAMData");
  if (it_core == props.end() || !it_core->value().is_object())
    return false;

  const auto &core = it_core->value().as_object();
  auto it_notam = core.find("notam");
  if (it_notam == core.end() || !it_notam->value().is_object())
    return false;

  const auto &notam = it_notam->value().as_object();
  auto it_id = notam.find("id");
  if (it_id == notam.end() || !it_id->value().is_string())
    return false;

  id = it_id->value().as_string().c_str();
  return !id.empty();
}

static bool
ApplyDeltaToApi(boost::json::value &api_response,
                const boost::json::value &delta_response,
                const std::vector<std::string> &removed_ids)
{
  if (!api_response.is_object() || !delta_response.is_object())
    return false;

  auto &api_obj = api_response.as_object();
  auto it_api_items = api_obj.find("items");
  if (it_api_items == api_obj.end() || !it_api_items->value().is_array())
    return false;

  auto &api_items = it_api_items->value().as_array();

  std::unordered_set<std::string> removed;
  if (!removed_ids.empty())
    removed.insert(removed_ids.begin(), removed_ids.end());

  if (!removed.empty()) {
    api_items.erase(std::remove_if(api_items.begin(), api_items.end(),
                                   [&removed](const boost::json::value &item) {
                                     std::string id;
                                     return GetNotamIdFromItem(item, id) &&
                                       removed.contains(id);
                                   }),
                    api_items.end());
  }

  std::unordered_map<std::string, std::size_t> index;
  index.reserve(api_items.size());
  for (std::size_t i = 0; i < api_items.size(); ++i) {
    std::string id;
    if (GetNotamIdFromItem(api_items[i], id))
      index.emplace(std::move(id), i);
  }

  const auto &delta_obj = delta_response.as_object();
  auto it_delta_items = delta_obj.find("items");
  if (it_delta_items != delta_obj.end() &&
      it_delta_items->value().is_array()) {
    const auto &delta_items = it_delta_items->value().as_array();
    for (const auto &item : delta_items) {
      std::string id;
      if (!GetNotamIdFromItem(item, id))
        continue;

      auto it = index.find(id);
      if (it != index.end()) {
        api_items[it->second] = item;
      } else {
        index.emplace(std::move(id), api_items.size());
        api_items.emplace_back(item);
      }
    }
  }

  if (api_obj.find("totalCount") != api_obj.end()) {
    api_obj["totalCount"] = static_cast<std::uint64_t>(api_items.size());
  }

  return true;
}

static bool
ParseJsonValue(const std::string &json, boost::json::value &value)
{
  boost::system::error_code ec;
  value = boost::json::parse(json, ec);
  if (ec) {
    LogFmt("NOTAM: JSON parse error: {}", ec.message());
    return false;
  }

  return true;
}

static bool
ParseNOTAMsFromApiValue(const boost::json::value &value,
                        std::vector<NOTAMStruct> &notams,
                        const char *context)
{
  const char *const safe_context =
    (context != nullptr && *context != '\0') ? context : "<unknown>";
  try {
    notams = NOTAMClient::ParseNOTAMGeoJSON(value);
    return true;
  } catch (const std::exception &e) {
    LogFmt("NOTAM: {} parse error: {}", safe_context, e.what());
    return false;
  }
}

NOTAMGlue::NOTAMGlue(const NOTAMSettings &_settings, CurlGlobal &_curl)
  : RateLimiter(std::chrono::seconds(30), std::chrono::seconds(30)),
    settings(_settings), curl(_curl), 
    current_notams_impl(std::make_unique<NOTAMImpl>()),
    load_task(curl.GetEventLoop())
{
}

NOTAMGlue::~NOTAMGlue() = default;

NOTAMSettings
NOTAMGlue::GetSettingsSnapshot() const noexcept
{
  const std::lock_guard<Mutex> lock(mutex);
  return settings;
}

void
NOTAMGlue::SetSettings(const NOTAMSettings &_settings) noexcept
{
  const std::lock_guard<Mutex> lock(mutex);
  settings = _settings;
}

std::chrono::system_clock::time_point
NOTAMGlue::GetCurrentTimeUTCSnapshot() const noexcept
{
  const std::lock_guard<Mutex> lock(mutex);
  return current_time_utc != std::chrono::system_clock::time_point{}
    ? current_time_utc
    : std::chrono::system_clock::now();
}

void
NOTAMGlue::OnTimer(const GeoPoint &current_location,
                   std::chrono::system_clock::time_point current_time)
{
  {
    const std::lock_guard<Mutex> lock(mutex);
    current_time_utc = current_time;
  }

  const auto settings_snapshot = GetSettingsSnapshot();

  if (!settings_snapshot.enabled) {
    LogDebug("NOTAM: Auto-refresh skipped - disabled in settings");
    return;
  }

  if (!current_location.IsValid()) {
    LogDebug("NOTAM: Auto-refresh skipped - invalid location");
    return;
  }

  // Check if manual-only mode (interval = 0)
  if (settings_snapshot.refresh_interval_min == 0) {
    LogDebug("NOTAM: Auto-refresh skipped - manual-only mode");
    return;
  }

  // If we're already loading or a retry is pending, skip auto-refresh
  std::time_t last_attempt_time_snapshot = 0;
  {
    const std::lock_guard<Mutex> lock(mutex);
    if (loading || retry_pending)
      return;
    last_attempt_time_snapshot = last_attempt_time;
  }

  GeoPoint last_loc = GetLastUpdateLocation();
  std::time_t last_time = GetLastUpdateTime();
  std::time_t now = std::time(nullptr);
  
  // Check if time interval has elapsed since last successful fetch
  bool time_expired = (last_time == 0) || 
                      (now - last_time >= static_cast<std::time_t>(
                                          settings_snapshot.refresh_interval_min * 60));

  const unsigned last_radius_km = GetLastUpdateRadius();
  const bool radius_changed =
    (last_radius_km > 0) && (last_radius_km != settings_snapshot.radius_km);
  
  // Also check if enough time has passed since last attempt (even if it
  // failed).
  // This prevents rapid retries when there's no network connection
  bool enough_time_since_attempt =
      (now - last_attempt_time_snapshot >=
       MIN_NOTAM_REFRESH_ATTEMPT_INTERVAL_SECONDS);
  
  // Check if location moved outside half the radius
  bool location_changed = last_loc.IsValid() &&
                          current_location.Distance(last_loc) >
                            (settings_snapshot.radius_km * 1000.0 / 2.0);
  
  if (!enough_time_since_attempt) {
    // Auto-refresh skipped - last attempt too recent
    return;
  }

  if (time_expired || location_changed || radius_changed) {
    UpdateLocation(current_location);
    return;
  }
}

void
NOTAMGlue::UpdateLocation(const GeoPoint &location)
{
  if (!location.IsValid()) {
    return;
  }

  // Check if already loading
  {
    const std::lock_guard<Mutex> lock(mutex);
    if (!settings.enabled || loading) {
      return; // Already loading, skip this request
    }
    loading = true;
    retry_pending = false;
    last_load_committed = false;
    current_location = location;
    last_attempt_time = std::time(nullptr);  // Record attempt time
  }
  
  // Log only when we actually start a fetch
  LogFmt("NOTAM: Auto-refresh starting");
  
  // Cancel any pending retry since we're starting a new attempt
  Cancel();

  // Start async loading
  load_task.Start(LoadNOTAMsInternal(location, false),
                  BIND_THIS_METHOD(OnLoadComplete));
}

bool
NOTAMGlue::ForceUpdateLocation(const GeoPoint &location,
                               const bool invalidate_cache_state)
{
  if (!location.IsValid())
    return false;

  {
    const std::lock_guard<Mutex> lock(mutex);
    if (!settings.enabled || loading)
      return false;

    loading = true;
    retry_pending = false;
    last_load_committed = false;
    current_location = location;
    last_attempt_time = std::time(nullptr);

    if (invalidate_cache_state) {
      auto *impl = current_notams_impl.get();
      impl->known.clear();
      impl->known_location = GeoPoint::Invalid();
      impl->known_radius_km = 0;
      impl->cached_api = boost::json::value();
      impl->cached_api_valid = false;
      impl->last_update = CacheMetadata{};
      impl->last_update_cached = false;
      ++mutation_generation;
    }
  }

  LogFmt("NOTAM: Manual refresh starting");
  Cancel();
  load_task.Start(LoadNOTAMsInternal(location, true),
                  BIND_THIS_METHOD(OnLoadComplete));
  return true;
}

void
NOTAMGlue::LoadNOTAMs(const GeoPoint &location)
{
  // Check if location is valid
  if (!location.IsValid()) {
    return;
  }
  
  // Check if already loading
  {
    const std::lock_guard lock(mutex);
    if (!settings.enabled || loading) {
      return;
    }
    loading = true;
    retry_pending = false;
    last_load_committed = false;
    current_location = location;
    last_attempt_time = std::time(nullptr);
  }
  
  // Cancel any pending retry since we're starting a new attempt
  Cancel();
  
  // Start async loading
  load_task.Start(LoadNOTAMsInternal(location, false),
                  BIND_THIS_METHOD(OnLoadComplete));
}

std::vector<NOTAMStruct>
NOTAMGlue::GetNOTAMs(unsigned max_count) const
{
  const std::lock_guard<Mutex> lock(mutex);
  auto *impl = current_notams_impl.get();
  
  const auto &notams = impl->current_notams;
  const unsigned available_count = static_cast<unsigned>(notams.size());
  
  // If max_count is 0 or greater than available, return all NOTAMs
  const unsigned count = (max_count == 0 || max_count > available_count) 
                         ? available_count : max_count;
  
  LogDebug("NOTAM: GetNOTAMs max_count={}, available={}, returning={}",
           max_count, available_count, count);
  
  // Return a copy of the requested NOTAMs
  std::vector<NOTAMStruct> result;
  result.reserve(count);
  result.insert(result.end(), notams.begin(), notams.begin() + count);
  
  return result;
}

static CacheMetadata GetCachedMetadata(NOTAMImpl &impl, Mutex &mutex,
                                       const AllocatedPath &file_path);

NOTAMGlue::Snapshot
NOTAMGlue::GetSnapshot() const
{
  const auto file_path = GetNOTAMCacheFilePath();
  (void)GetCachedMetadata(*current_notams_impl, mutex, file_path);

  Snapshot snapshot;
  {
    const std::lock_guard<Mutex> lock(mutex);
    const auto &meta = current_notams_impl->last_update;
    snapshot.notams = current_notams_impl->current_notams;
    snapshot.last_update_time = meta.timestamp;
    snapshot.last_update_location = meta.location;
  }
  return snapshot;
}
void
NOTAMGlue::Clear()
{
  const std::lock_guard<Mutex> lock(mutex);
  auto *impl = current_notams_impl.get();
  impl->current_notams.clear();
  impl->known.clear();
  impl->known_location = GeoPoint::Invalid();
  impl->known_radius_km = 0;
  impl->cached_api = boost::json::value();
  impl->cached_api_valid = false;
  impl->last_update = CacheMetadata{};
  impl->last_update_cached = false;
  manual_refresh_requested = false;
  ++mutation_generation;
}

// Duplicate method removed

Co::InvokeTask
NOTAMGlue::LoadNOTAMsInternal(GeoPoint location, bool force_network)
{
  const uint64_t request_generation = [this]() {
    const std::lock_guard<Mutex> lock(mutex);
    return mutation_generation;
  }();
  const auto settings_snapshot = GetSettingsSnapshot();

  LogFmt("NOTAM: Starting LoadNOTAMsInternal");

  const auto can_commit = [this, request_generation]() {
    const std::lock_guard<Mutex> lock(mutex);
    return settings.enabled && request_generation == mutation_generation;
  };

  const auto consume_manual_refresh_requested = [this]() {
    const std::lock_guard<Mutex> lock(mutex);
    const bool value = manual_refresh_requested;
    manual_refresh_requested = false;
    return value;
  };

  try {
    // First, try to load from cache if not expired
    LogFmt("NOTAM: Checking cache expiration status");
    if (!force_network && !IsCacheExpired()) {
      LogFmt("NOTAM: Cache is still valid, attempting to load from cache");
      std::vector<NOTAMStruct> cached_notams;
      GeoPoint cache_location = GeoPoint::Invalid();
      unsigned cache_radius_km = 0;
      boost::json::value cached_api;
      if (LoadNOTAMsFromFile(cached_notams, &cache_location, &cache_radius_km,
                             &cached_api)) {
        const unsigned count = static_cast<unsigned>(cached_notams.size());
        LogFmt("NOTAM: Successfully loaded {} NOTAMs from cache", count);

        if (!can_commit())
          co_return;

        const GeoPoint known_location =
          cache_location.IsValid() ? cache_location : location;
        const unsigned known_radius_km =
          cache_radius_km > 0 ? cache_radius_km : settings_snapshot.radius_km;

        // Store cached results
        {
          const std::lock_guard<Mutex> lock(mutex);
          if (!settings.enabled || request_generation != mutation_generation)
            co_return;

          auto *impl = current_notams_impl.get();
          PopulateImplFromCache(*impl, std::move(cached_notams),
                                known_location, known_radius_km,
                                std::move(cached_api));
          last_load_committed = true;
        }
        LogFmt("NOTAM: Using cached data, fetch complete");
        co_return; // Use cached data, no need to fetch
      } else {
        LogFmt("NOTAM: Failed to load from cache file, will fetch fresh "
               "data");
      }
    } else {
      LogFmt("NOTAM: Cache is expired or doesn't exist, fetching fresh "
             "data");
    }
    
    // Cache is expired or doesn't exist, fetch fresh data from API
    LogFmt("NOTAM: Starting API fetch for radius {} km",
           settings_snapshot.radius_km);

    // Use a null progress listener
    NullOperationEnvironment progress;

    NOTAMClient::KnownMap known_copy;
    {
      const std::lock_guard<Mutex> lock(mutex);
      auto *impl = current_notams_impl.get();
      if (CanUseDelta(*impl, location, settings_snapshot.radius_km))
        known_copy = impl->known;
    }

    if (!known_copy.empty()) {
      try {
        LogFmt("NOTAM: Attempting delta fetch with {} known IDs",
               static_cast<unsigned>(known_copy.size()));
        auto delta_response =
          co_await NOTAMClient::FetchNOTAMsResponse(curl, settings_snapshot,
                                                    location,
                                                    progress, &known_copy);

        if (delta_response.is_delta) {
          boost::json::value delta_json;
          if (!ParseJsonValue(delta_response.raw_json, delta_json)) {
            LogFmt("NOTAM: Delta response JSON parse failed, falling back");
          } else {
            boost::json::value api_snapshot;
            unsigned total = 0;
            bool applied = false;
            {
              const std::lock_guard<Mutex> lock(mutex);
              if (!settings.enabled || request_generation != mutation_generation)
                co_return;

              auto *impl = current_notams_impl.get();
              if (impl->cached_api_valid) {
                auto cached_api = impl->cached_api;
                auto current_notams = impl->current_notams;

                if (ApplyDeltaToApi(cached_api, delta_json,
                                    delta_response.removed_ids)) {
                  ApplyDeltaUpdates(current_notams, delta_response.notams,
                                    delta_response.removed_ids);
                  auto known = BuildKnownMap(current_notams);
                  api_snapshot = cached_api;
                  total = static_cast<unsigned>(current_notams.size());

                  impl->cached_api = std::move(cached_api);
                  impl->current_notams = std::move(current_notams);
                  impl->known = std::move(known);
                  impl->known_location = location;
                  impl->known_radius_km = settings_snapshot.radius_km;
                  impl->last_update.timestamp = std::time(nullptr);
                  impl->last_update.location = location;
                  impl->last_update.radius_km = settings_snapshot.radius_km;
                  impl->last_update.valid = true;
                  impl->last_update_cached = true;
                  last_load_committed = true;
                  applied = true;
                }
              }
            }

            if (applied) {
              if (!can_commit())
                co_return;

              SaveNOTAMsToFile(api_snapshot, location, request_generation);

              LogFmt("NOTAM: Delta applied (updates={}, removed={}, total={})",
                     static_cast<unsigned>(delta_response.notams.size()),
                     static_cast<unsigned>(
                       delta_response.removed_ids.size()),
                     total);

              if (consume_manual_refresh_requested())
                AddLoadedNOTAMsMessage(total);

              co_return;
            }

            LogFmt("NOTAM: Delta merge failed, falling back to full fetch");
          }
        } else {
          boost::json::value api_value;
          if (ParseJsonValue(delta_response.raw_json, api_value) &&
              IsApiResponseValid(api_value)) {
            unsigned total = 0;

            if (!can_commit())
              co_return;

            {
              const std::lock_guard<Mutex> lock(mutex);
              if (!settings.enabled || request_generation != mutation_generation)
                co_return;

              auto *impl = current_notams_impl.get();
              impl->current_notams = std::move(delta_response.notams);
              impl->known = BuildKnownMap(impl->current_notams);
              impl->known_location = location;
              impl->known_radius_km = settings_snapshot.radius_km;
              impl->cached_api = api_value;
              impl->cached_api_valid = true;
              impl->last_update.timestamp = std::time(nullptr);
              impl->last_update.location = location;
              impl->last_update.radius_km = settings_snapshot.radius_km;
              impl->last_update.valid = true;
              impl->last_update_cached = true;
              last_load_committed = true;
              total = static_cast<unsigned>(impl->current_notams.size());
            }

            SaveNOTAMsToFile(api_value, location, request_generation);

            LogFmt("NOTAM: Full response accepted (total={})",
                   total);

            if (consume_manual_refresh_requested())
              AddLoadedNOTAMsMessage(total);

            co_return;
          }

          LogFmt("NOTAM: Delta response missing full API payload");
        }

        LogFmt("NOTAM: Delta not applied, falling back to full fetch");
        {
          const std::lock_guard<Mutex> lock(mutex);
          auto *impl = current_notams_impl.get();
          impl->known.clear();
          impl->known_location = GeoPoint::Invalid();
          impl->known_radius_km = 0;
        }
      } catch (const std::exception &e) {
        LogFmt("NOTAM: Delta fetch failed: {}", e.what());
        {
          const std::lock_guard<Mutex> lock(mutex);
          auto *impl = current_notams_impl.get();
          impl->known.clear();
          impl->known_location = GeoPoint::Invalid();
          impl->known_radius_km = 0;
        }
      }
    }

    LogFmt("NOTAM: Performing full fetch");
    auto full_response =
      co_await NOTAMClient::FetchNOTAMsResponse(curl, settings_snapshot,
                                                location,
                                                progress, nullptr);
    boost::json::value api_value;
    const bool parsed = ParseJsonValue(full_response.raw_json, api_value);
    const bool api_valid = parsed && IsApiResponseValid(api_value);
    if (!api_valid) {
      LogFmt("NOTAM: Rejecting invalid API payload (parsed={}, bytes={})",
             parsed ? 1 : 0,
             static_cast<unsigned>(full_response.raw_json.size()));
      throw std::runtime_error("Invalid NOTAM API payload");
    }

    auto notams = std::move(full_response.notams);
    const unsigned count = static_cast<unsigned>(notams.size());
    LogFmt("NOTAM: Parsed {} NOTAMs from response", count);

    if (!can_commit())
      co_return;

    {
      const std::lock_guard<Mutex> lock(mutex);
      if (!settings.enabled || request_generation != mutation_generation)
        co_return;

      auto *impl = current_notams_impl.get();
      impl->current_notams = std::move(notams);
      impl->known = BuildKnownMap(impl->current_notams);
      impl->known_location = location;
      impl->known_radius_km = settings_snapshot.radius_km;
      impl->cached_api = api_value;
      impl->cached_api_valid = true;
      impl->last_update.timestamp = std::time(nullptr);
      impl->last_update.location = location;
      impl->last_update.radius_km = settings_snapshot.radius_km;
      impl->last_update.valid = true;
      impl->last_update_cached = true;
      last_load_committed = true;
    }

    SaveNOTAMsToFile(api_value, location, request_generation);
    LogFmt("NOTAM: Cache file saved successfully");

    LogFmt("NOTAM: Successfully completed fetch with {} NOTAMs", count);

    if (consume_manual_refresh_requested())
      AddLoadedNOTAMsMessage(count);
    
  } catch (const std::exception &e) {
    LogFmt("NOTAM: Error during fetch: {}", e.what());

    if (!can_commit()) {
      LogFmt("NOTAM: Ignoring stale fetch error");
      co_return;
    }
    
    // Keep existing NOTAMs on fetch failure - stale data is better than no
    // data.
    LogFmt("NOTAM: Keeping existing NOTAMs after fetch error");
    
    bool show_error_message = false;
    {
      const std::lock_guard<Mutex> lock(mutex);
      manual_refresh_requested = false;
      if (!fetch_failure_notified) {
        fetch_failure_notified = true;
        show_error_message = true;
      }
    }

    if (show_error_message)
      Message::AddMessage(_("Failed to load NOTAMs"));

    throw;
  }
}

void
NOTAMGlue::OnLoadComplete(std::exception_ptr error) noexcept
{
  // Reset loading flag
  bool load_failed = false;
  bool load_committed = false;
  bool schedule_retry = false;
  {
    const std::lock_guard<Mutex> lock(mutex);
    loading = false;
    load_committed = last_load_committed;
    last_load_committed = false;
    if (error) {
      load_failed = true;
      schedule_retry = settings.refresh_interval_min > 0;
      retry_pending = schedule_retry;
    } else if (!load_committed) {
      retry_pending = false;
    } else {
      retry_pending = false;
      fetch_failure_notified = false;
    }
  }

  if (load_failed) {
    {
      const std::lock_guard<Mutex> lock(mutex);
      listener_load_complete_pending = false;
    }
    listener_notify.SendNotification();

    if (schedule_retry) {
      // Failed - schedule retry with fixed 30-second delay
      LogFmt("NOTAM: Fetch failed, scheduling retry in 30 seconds");
      Trigger();
    } else {
      Cancel();
    }
  } else if (!load_committed) {
    Cancel();
  } else {
    // Success - cancel any pending retry
    Cancel();
    
    // Update the airspace database
    if (data_components && data_components->airspaces) {
      try {
        LogFmt("NOTAM: Updating airspace database with loaded NOTAMs");
        const ScopeSuspendAllThreads suspend;
        UpdateAirspaces(*data_components->airspaces);
        if (data_components->terrain != nullptr)
          SetAirspaceGroundLevels(*data_components->airspaces,
                                  *data_components->terrain);
        LogFmt("NOTAM: Airspace database updated successfully");
      } catch (const std::exception &e) {
        LogFmt("NOTAM: Error updating airspace database: {}", e.what());
      }
    }
    
    // Notify listeners that NOTAMs have been updated
    NotifyListeners();

    {
      const std::lock_guard<Mutex> lock(mutex);
      listener_load_complete_pending = true;
    }
    listener_notify.SendNotification();
  }
}

void
NOTAMGlue::ResetFetchFailureNotification() noexcept
{
  const std::lock_guard<Mutex> lock(mutex);
  fetch_failure_notified = false;
}

void
NOTAMGlue::MarkManualRefreshRequested() noexcept
{
  const std::lock_guard<Mutex> lock(mutex);
  manual_refresh_requested = true;
}

void
NOTAMGlue::AddListener(NOTAMListener &listener)
{
  const std::lock_guard<Mutex> lock(mutex);
  listeners.push_back(&listener);
}

void
NOTAMGlue::RemoveListener(NOTAMListener &listener) noexcept
{
  const std::lock_guard<Mutex> lock(mutex);
  auto it = std::find(listeners.begin(), listeners.end(), &listener);
  if (it != listeners.end())
    listeners.erase(it);
}

void
NOTAMGlue::NotifyListeners() noexcept
{
  {
    const std::lock_guard<Mutex> lock(mutex);
    listener_update_pending = true;
  }
  listener_notify.SendNotification();
}

void
NOTAMGlue::OnListenerNotification() noexcept
{
  bool notify_updated = false;
  std::optional<bool> notify_load_complete;
  {
    const std::lock_guard<Mutex> lock(mutex);
    notify_updated = listener_update_pending;
    listener_update_pending = false;
    notify_load_complete = listener_load_complete_pending;
    listener_load_complete_pending.reset();
  }

  if (notify_updated) {
    std::vector<NOTAMListener *> listeners_copy;
    try {
      const std::lock_guard<Mutex> lock(mutex);
      listeners_copy = listeners;
    } catch (const std::exception &e) {
      LogFmt("NOTAM: Failed to copy listeners: {}", e.what());
      listeners_copy.clear();
    } catch (...) {
      LogFmt("NOTAM: Failed to copy listeners");
      listeners_copy.clear();
    }

    for (auto *listener : listeners_copy)
      listener->OnNOTAMsUpdated();
  }

  if (notify_load_complete.has_value()) {
    std::vector<NOTAMListener *> listeners_copy;
    try {
      const std::lock_guard<Mutex> lock(mutex);
      listeners_copy = listeners;
    } catch (const std::exception &e) {
      LogFmt("NOTAM: Failed to copy load-complete listeners: {}",
             e.what());
      return;
    } catch (...) {
      LogFmt("NOTAM: Failed to copy load-complete listeners");
      return;
    }

    for (auto *listener : listeners_copy)
      listener->OnNOTAMsLoadComplete(*notify_load_complete);
  }
}

void
NOTAMGlue::Run()
{
  LogFmt("NOTAM: Retry timer fired, attempting fetch again");
  
  GeoPoint location;
  {
    const std::lock_guard<Mutex> lock(mutex);
    retry_pending = false;
    if (loading || !current_location.IsValid()) {
      return;
    }
    location = current_location;
  }
  
  // Trigger a new fetch attempt (outside the lock)
  UpdateLocation(location);
}

unsigned
NOTAMGlue::LoadCachedNOTAMs()
{
  std::vector<NOTAMStruct> cached_notams;
  GeoPoint cache_location = GeoPoint::Invalid();
  unsigned cache_radius_km = 0;
  boost::json::value cached_api;
  
  LogFmt("NOTAM: Attempting to load NOTAM cache");
  
  // Try to load from cache file
  if (LoadNOTAMsFromFile(cached_notams, &cache_location, &cache_radius_km,
                         &cached_api)) {
    const unsigned count = static_cast<unsigned>(cached_notams.size());
    
    // Store cached results in memory
    {
      const std::lock_guard<Mutex> lock(mutex);
      auto *impl = current_notams_impl.get();
      PopulateImplFromCache(
        *impl, std::move(cached_notams),
        cache_location.IsValid() ? cache_location : GeoPoint::Invalid(),
        cache_radius_km, std::move(cached_api));
    }
        
    LogFmt("NOTAM: Loaded {} NOTAMs from cache", count);
    return count;
  }
  
  LogFmt("NOTAM: No cached NOTAMs found");
  return 0;
}

bool
NOTAMGlue::LoadCachedNOTAMsAndUpdate(Airspaces &airspaces,
                                     const GeoPoint &current_location)
{
  const auto settings_snapshot = GetSettingsSnapshot();
  if (!settings_snapshot.enabled) {
    LogDebug("NOTAM: Startup cache load skipped - disabled in settings");
    return false;
  }

  std::vector<NOTAMStruct> cached_notams;
  GeoPoint cache_location = GeoPoint::Invalid();
  unsigned cache_radius_km = 0;
  boost::json::value cached_api;
  if (!LoadNOTAMsFromFile(cached_notams, &cache_location, &cache_radius_km,
                          &cached_api)) {
    LogFmt("NOTAM: No cached NOTAMs to apply");
    return false;
  }

  if (cache_radius_km != settings_snapshot.radius_km) {
    LogFmt("NOTAM: Startup cache replay skipped - cached radius {} km "
           "does not match current radius {} km",
           cache_radius_km, settings_snapshot.radius_km);
    return false;
  }

  if (!cache_location.IsValid()) {
    LogFmt("NOTAM: Startup cache replay skipped - missing location metadata");
    return false;
  }

  if (current_location.IsValid() &&
      current_location.Distance(cache_location) >
        static_cast<double>(cache_radius_km) * 1000.) {
    LogFmt("NOTAM: Startup cache replay skipped - current location outside "
           "cached coverage");
    return false;
  }

  const unsigned count = static_cast<unsigned>(cached_notams.size());
  {
    const std::lock_guard<Mutex> lock(mutex);
    auto *impl = current_notams_impl.get();
    PopulateImplFromCache(
      *impl, std::move(cached_notams),
      cache_location.IsValid() ? cache_location : GeoPoint::Invalid(),
      cache_radius_km, std::move(cached_api));
  }

  LogFmt("NOTAM: Loaded {} NOTAMs from cache for startup", count);

  UpdateAirspaces(airspaces);
  NotifyListeners();

  return true;
}

void
NOTAMGlue::InvalidateCache()
{
  auto file_path = GetNOTAMCacheFilePath();
  
  LogFmt("NOTAM: Invalidating NOTAM cache file");
  NOTAMCache::InvalidateFile(file_path);

  {
    const std::lock_guard<Mutex> lock(mutex);
    auto *impl = current_notams_impl.get();
    impl->known.clear();
    impl->known_location = GeoPoint::Invalid();
    impl->known_radius_km = 0;
    impl->cached_api = boost::json::value();
    impl->cached_api_valid = false;
    impl->last_update = CacheMetadata{};
    impl->last_update_cached = false;
    manual_refresh_requested = false;
    ++mutation_generation;
  }
}

unsigned
NOTAMGlue::GetTotalCount() const
{
  const std::lock_guard<Mutex> lock(mutex);
  auto *impl = current_notams_impl.get();
  return static_cast<unsigned>(impl->current_notams.size());
}

unsigned
NOTAMGlue::GetFilteredCount() const
{
  const auto now = GetCurrentTimeUTCSnapshot();
  const std::lock_guard<Mutex> lock(mutex);
  auto *impl = current_notams_impl.get();
  
  unsigned count = 0;
  for (const auto &notam : impl->current_notams) {
    if (NOTAMFilter::ShouldDisplay(notam, settings, now, false)) {
      count++;
    }
  }
  return count;
}

NOTAMGlue::FilterStats
NOTAMGlue::GetFilterStats() const
{
  const auto now = GetCurrentTimeUTCSnapshot();
  const std::lock_guard<Mutex> lock(mutex);
  auto *impl = current_notams_impl.get();
  
  FilterStats stats;
  stats.total = static_cast<unsigned>(impl->current_notams.size());
  stats.filtered_by_ifr = 0;
  stats.filtered_by_time = 0;
  stats.filtered_by_qcode = 0;
  stats.filtered_by_radius = 0;
  stats.final_count = 0;

  const char *hidden_list = settings.hidden_qcodes.c_str();
  
  for (const auto &notam : impl->current_notams) {
    // Count how many fail IFR filter (independently)
    if (!settings.show_ifr && !notam.traffic.empty() && notam.traffic == "I") {
      stats.filtered_by_ifr++;
    }
    
    // Count how many fail time filter (independently)
    if (settings.show_only_effective) {
      if (!notam.IsActive(now)) {
        stats.filtered_by_time++;
      }
    }
    
    // Count how many fail Q-code filter (independently)
    const auto &qcode = notam.feature_type;
    if (!qcode.empty() && NOTAMFilter::IsQCodeHidden(qcode, hidden_list)) {
      stats.filtered_by_qcode++;
    }
    
    // Count how many fail radius filter (independently)
    if (settings.max_radius_m > 0) {
      if (notam.geometry.radius_meters > settings.max_radius_m) {
        stats.filtered_by_radius++;
      }
    }
    
    // Use the main filter function for final count
    if (NOTAMFilter::ShouldDisplay(notam, settings, now, false)) {
      stats.final_count++;
    }
  }
  
  return stats;
}

std::optional<struct NOTAM>
NOTAMGlue::FindNOTAMByNumber(const std::string &number) const
{
  const std::lock_guard<Mutex> lock(mutex);
  auto *impl = current_notams_impl.get();
  
  for (const auto &notam : impl->current_notams) {
    if (notam.number == number || notam.id == number) {
      return notam; // Return a copy while holding the lock
    }
  }
  
  return std::nullopt;
}

void
NOTAMGlue::UpdateAirspaces(Airspaces &airspaces)
{
  const auto now = GetCurrentTimeUTCSnapshot();
  const std::lock_guard<Mutex> lock(mutex);
  auto *impl = current_notams_impl.get();

  LogFmt("NOTAM: UpdateAirspaces - converting {} NOTAMs to airspaces",
         static_cast<unsigned>(impl->current_notams.size()));

  // Save all non-NOTAM airspaces
  // ToDo: Optimize by removing only existing NOTAM airspaces
  std::vector<AirspacePtr> saved_airspaces;
  for (const auto &airspace : airspaces.QueryAll()) {
    const auto &current = airspace.GetAirspace();
    if (current.GetType() != AirspaceClass::NOTAM) {
      saved_airspaces.push_back(airspace.GetAirspacePtr());
    }
  }

  LogFmt("NOTAM: Saved {} non-NOTAM airspaces",
         static_cast<unsigned>(saved_airspaces.size()));

  // Clear all airspaces
  airspaces.Clear();

  // Restore non-NOTAM airspaces
  for (const auto &airspace : saved_airspaces) {
    airspaces.Add(airspace);
  }

  LogFmt("NOTAM: Restored {} non-NOTAM airspaces",
         static_cast<unsigned>(saved_airspaces.size()));

  // Add filtered NOTAM airspaces
  unsigned added_count = 0;
  unsigned filtered_count = 0;

  for (const auto &notam : impl->current_notams) {
    try {
      // Apply filtering
      if (!NOTAMFilter::ShouldDisplay(notam, settings, now, false)) {
        filtered_count++;
        continue;
      }

      if (NOTAMConverter::ConvertNOTAMToAirspace(notam, airspaces))
        added_count++;
    } catch (const std::exception &e) {
      LogFmt("NOTAM: Error creating airspace for NOTAM '{}': {}",
             notam.number.c_str(), e.what());
    }
  }

  // Optimize the airspace tree after adding NOTAMs
  airspaces.Optimise();

  LogFmt("NOTAM: UpdateAirspaces completed - added {} NOTAMs, filtered {} NOTAMs",
         added_count, filtered_count);
}

AllocatedPath
NOTAMGlue::GetNOTAMCacheFilePath() const
{
  return NOTAMCache::GetFilePath();
}

static CacheMetadata
GetCachedMetadata(NOTAMImpl &impl, Mutex &mutex,
                  const AllocatedPath &file_path)
{
  {
    const std::lock_guard<Mutex> lock(mutex);
    if (impl.last_update_cached)
      return impl.last_update;
  }

  const CacheMetadata meta =
    NOTAMCache::LoadMetadataFromFile(file_path).value_or(CacheMetadata{});

  const std::lock_guard<Mutex> lock(mutex);
  if (!impl.last_update_cached) {
    impl.last_update = meta;
    impl.last_update_cached = true;
  }
  return impl.last_update;
}

void
NOTAMGlue::SaveNOTAMsToFile(const boost::json::value &api_response,
                             const GeoPoint &location,
                             const uint64_t expected_generation) const
{
  if (!IsApiResponseValid(api_response)) {
    LogFmt("NOTAM: Skipping cache save (invalid API response)");
    return;
  }

  NOTAMSettings settings_snapshot;
  {
    const std::lock_guard<Mutex> lock(mutex);
    if (!settings.enabled || mutation_generation != expected_generation) {
      LogFmt("NOTAM: Skipping cache save (stale generation)");
      return;
    }
    settings_snapshot = settings;
  }

  const auto validate_commit = [this, expected_generation]() {
    const std::lock_guard<Mutex> lock(mutex);
    return settings.enabled && mutation_generation == expected_generation;
  };

  LogFmt("NOTAM: Saving NOTAM cache");
  try {
    NOTAMCache::SaveToFile(GetNOTAMCacheFilePath(), api_response, location,
                           settings_snapshot.radius_km,
                           settings_snapshot.refresh_interval_min,
                           validate_commit);
  } catch (const std::exception &e) {
    LogFmt("NOTAM: Error saving NOTAM cache to file: {}", e.what());
  }
}

bool
NOTAMGlue::LoadNOTAMsFromFile(std::vector<NOTAMStruct> &notams,
                               GeoPoint *location,
                               unsigned *radius_km,
                               boost::json::value *api_response) const
{
  try {
    const auto file_path = GetNOTAMCacheFilePath();
    LogFmt("NOTAM: LoadNOTAMsFromFile attempting to load cache");

    boost::json::value root;
    if (!NOTAMCache::LoadJsonValue(file_path, root) || !root.is_object()) {
      LogFmt("NOTAM: LoadNOTAMsFromFile failed to load or parse file");
      return false;
    }

    const auto &obj = root.as_object();
    const CacheMetadata meta = NOTAMCache::ExtractMetadata(obj);

    if (location != nullptr)
      *location = meta.location;
    if (radius_km != nullptr)
      *radius_km = meta.radius_km;
    if (api_response != nullptr)
      *api_response = boost::json::value();

    auto it = obj.find("api");
    if (it == obj.end()) {
      LogFmt("NOTAM: LoadNOTAMsFromFile missing api payload");
      return false;
    }

    const auto &api_value = it->value();
    if (api_response != nullptr)
      *api_response = api_value;

    if (!IsApiResponseValid(api_value)) {
      LogFmt("NOTAM: LoadNOTAMsFromFile invalid api payload");
      return false;
    }

    notams.clear();
    if (!ParseNOTAMsFromApiValue(api_value, notams, "api"))
      return false;

    LogFmt("NOTAM: LoadNOTAMsFromFile parsed {} NOTAMs",
           static_cast<unsigned>(notams.size()));
    return true;
  } catch (const std::exception &e) {
    LogFmt("NOTAM: LoadNOTAMsFromFile exception: {}", e.what());
    return false;
  }
}

std::time_t
NOTAMGlue::GetLastUpdateTime() const
{
  const auto file_path = GetNOTAMCacheFilePath();
  const auto meta =
    GetCachedMetadata(*current_notams_impl, mutex, file_path);
  return meta.timestamp;
}

GeoPoint
NOTAMGlue::GetLastUpdateLocation() const
{
  const auto file_path = GetNOTAMCacheFilePath();
  const auto meta =
    GetCachedMetadata(*current_notams_impl, mutex, file_path);
  return meta.location;
}

unsigned
NOTAMGlue::GetLastUpdateRadius() const
{
  const auto file_path = GetNOTAMCacheFilePath();
  const auto meta =
    GetCachedMetadata(*current_notams_impl, mutex, file_path);
  return meta.radius_km;
}

bool
NOTAMGlue::IsCacheExpired() const
{
  try {
    const auto settings_snapshot = GetSettingsSnapshot();
    const auto file_path = GetNOTAMCacheFilePath();
    const auto meta =
      GetCachedMetadata(*current_notams_impl, mutex, file_path);

    if (meta.timestamp <= 0)
      return true;

    if (!meta.location.IsValid())
      return true;

    if (meta.radius_km == 0)
      return true;

    std::time_t current_time = std::time(nullptr);
    std::time_t max_age_seconds = settings_snapshot.refresh_interval_min * 60;

    if (max_age_seconds > 0 &&
        (current_time - meta.timestamp) > max_age_seconds)
      return true;

    if (meta.radius_km != settings_snapshot.radius_km)
      return true;

    GeoPoint location_copy;
    {
      const std::lock_guard<Mutex> lock(mutex);
      location_copy = current_location;
    }

    if (location_copy.IsValid()) {
      double distance_m = location_copy.Distance(meta.location);
      // IsCacheExpired() treats cache as valid for the whole configured
      // search area around meta.location (full settings.radius_km).
      // OnTimer() intentionally triggers earlier at half radius to refresh
      // proactively before crossing the hard cache-validity boundary.
      double radius_m = settings_snapshot.radius_km * 1000.0;
      if (distance_m > radius_m)
        return true;
    }

    return false;
  } catch (const std::exception &) {
    return true; // Error means treat as expired
  }
}

#endif // HAVE_HTTP
