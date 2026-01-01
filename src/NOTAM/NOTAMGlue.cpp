// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The XCSoar Project

#include "NOTAMGlue.hpp"
#include "NOTAM.hpp"
#include "Client.hpp"
#include "Filter.hpp"
#include "Components.hpp"
#include "DataComponents.hpp"
#include "Message.hpp"
#include "Airspace/Airspaces.hpp"
#include "Airspace/AirspaceCircle.hpp"
#include "Airspace/AirspacePolygon.hpp"
#include "Airspace/AirspaceClass.hpp"
#include "Airspace/AirspaceAltitude.hpp"
#include "TransponderCode.hpp"
#include "Operation/Operation.hpp"
#include "Operation/ProgressListener.hpp"
#include "io/FileOutputStream.hxx"
#include "io/FileReader.hxx"
#include "system/Path.hpp"
#include "system/FileUtil.hpp"
#include "LocalPath.hpp"
#include "LogFile.hpp"
#include "util/StaticString.hxx"
#include "util/ConvertString.hpp"
#include "util/SpanCast.hxx"
#include "util/tstring.hpp"
#include "util/UTF8.hpp"
#include "Geo/AltitudeReference.hpp"
#include "thread/Mutex.hxx"
#include "co/Task.hxx"
#include "co/InvokeTask.hxx"
#include "lib/curl/Global.hxx"
#include "time/Convert.hxx"
#include "Language/Language.hpp"

#include <optional>
#include <algorithm>
#include <charconv>
#include <cstdio>
#include <cmath>
#include <cstring>
#include <string_view>
#include <system_error>
#include <unordered_map>
#include <unordered_set>

#include <boost/json.hpp>
#include <boost/json/serializer.hpp>

// Use full struct name to avoid collision with AirspaceClass::NOTAM enum
using NOTAMStruct = struct NOTAM;

#ifdef HAVE_HTTP

static constexpr std::time_t MIN_NOTAM_REFRESH_ATTEMPT_INTERVAL_SECONDS =
    120;
// Sentinel for missing MSL altitude in NOTAM data.
static constexpr double MISSING_MSL_ALTITUDE = -1.0;
// Truncate NOTAM text for airspace name display.
static constexpr size_t NOTAM_TEXT_TRUNCATE_LENGTH = 150;

struct CacheMetadata {
  std::time_t timestamp = 0;
  GeoPoint location = GeoPoint::Invalid();
  unsigned radius_km = 0;
  bool valid = false;
};

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
                                     removed.find(notam.id) != removed.end();
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
                                       removed.find(id) != removed.end();
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
    LogFormat("NOTAM: JSON parse error: %s", ec.message().c_str());
    return false;
  }

  return true;
}

static std::string
SerializeJsonValueReadable(const boost::json::value &value);

static bool
ParseNOTAMsFromApiValue(const boost::json::value &value,
                        std::vector<NOTAMStruct> &notams,
                        const char *context)
{
  try {
    notams = NOTAMClient::ParseNOTAMGeoJSON(value);
    return true;
  } catch (const std::exception &e) {
    LogFormat("NOTAM: %s parse error: %s", context, e.what());
    return false;
  }
}

static void
AppendJsonString(std::string &out, boost::json::string_view text)
{
  boost::json::serializer s;
  boost::json::value value(text);
  s.reset(&value);

  while (!s.done()) {
    char buffer[BOOST_JSON_STACK_BUFFER_SIZE];
    auto r = s.read(buffer);
    out.append(r.data(), r.size());
  }
}

static void
AppendJsonInteger(std::string &out, std::int64_t value)
{
  char buffer[32];
  auto result = std::to_chars(buffer, buffer + sizeof(buffer), value);
  if (result.ec == std::errc()) {
    out.append(buffer, result.ptr - buffer);
  } else {
    LogFormat("NOTAM: Integer conversion error for value %lld",
              static_cast<long long>(value));
    out.append("0");
  }
}

static void
AppendJsonUnsigned(std::string &out, std::uint64_t value)
{
  char buffer[32];
  auto result = std::to_chars(buffer, buffer + sizeof(buffer), value);
  if (result.ec == std::errc()) {
    out.append(buffer, result.ptr - buffer);
  } else {
    LogFormat("NOTAM: Unsigned conversion error for value %llu",
              static_cast<unsigned long long>(value));
    out.append("0");
  }
}

static void
AppendJsonDouble(std::string &out, double value)
{
  if (!std::isfinite(value)) {
    out.append("null");
    return;
  }

  if (value == 0.0) {
    out.push_back('0');
    return;
  }

  char buffer[128];
  int length = std::snprintf(buffer, sizeof(buffer), "%.17f", value);
  if (length <= 0) {
    out.append("0");
    return;
  }

  std::size_t trimmed = static_cast<std::size_t>(length);
  std::size_t dot = std::string_view(buffer, trimmed).find('.');
  if (dot != std::string_view::npos) {
    while (trimmed > dot + 1 && buffer[trimmed - 1] == '0')
      --trimmed;
    if (trimmed > dot && buffer[trimmed - 1] == '.')
      --trimmed;
  }

  if (trimmed == 0)
    out.append("0");
  else
    out.append(buffer, trimmed);
}

static void
AppendJsonValue(std::string &out, const boost::json::value &value)
{
  switch (value.kind()) {
  case boost::json::kind::object: {
    out.push_back('{');
    bool first = true;
    for (const auto &entry : value.as_object()) {
      if (!first)
        out.push_back(',');
      first = false;
      AppendJsonString(out, entry.key());
      out.push_back(':');
      AppendJsonValue(out, entry.value());
    }
    out.push_back('}');
    break;
  }
  case boost::json::kind::array: {
    out.push_back('[');
    bool first = true;
    for (const auto &entry : value.as_array()) {
      if (!first)
        out.push_back(',');
      first = false;
      AppendJsonValue(out, entry);
    }
    out.push_back(']');
    break;
  }
  case boost::json::kind::string:
    AppendJsonString(out, value.as_string());
    break;
  case boost::json::kind::int64:
    AppendJsonInteger(out, value.as_int64());
    break;
  case boost::json::kind::uint64:
    AppendJsonUnsigned(out, value.as_uint64());
    break;
  case boost::json::kind::double_:
    AppendJsonDouble(out, value.as_double());
    break;
  case boost::json::kind::bool_:
    out.append(value.as_bool() ? "true" : "false");
    break;
  case boost::json::kind::null:
    out.append("null");
    break;
  }
}

static std::string
SerializeJsonValueReadable(const boost::json::value &value)
{
  std::string out;
  out.reserve(4096);
  AppendJsonValue(out, value);
  return out;
}


NOTAMGlue::NOTAMGlue(const NOTAMSettings &_settings, CurlGlobal &_curl)
  : RateLimiter(std::chrono::seconds(30), std::chrono::seconds(30)),
    settings(_settings), curl(_curl), 
    current_notams_impl(std::make_unique<NOTAMImpl>()),
    load_task(curl.GetEventLoop())
{
}

NOTAMGlue::~NOTAMGlue() = default;

void
NOTAMGlue::OnTimer(const GeoPoint &current_location)
{
  if (!settings.enabled) {
    LogDebug("NOTAM: Auto-refresh skipped - disabled in settings");
    return;
  }

  if (!current_location.IsValid()) {
    LogDebug("NOTAM: Auto-refresh skipped - invalid location");
    return;
  }

  // Check if manual-only mode (interval = 0)
  if (settings.refresh_interval_min == 0) {
    LogDebug("NOTAM: Auto-refresh skipped - manual-only mode");
    return;
  }

  // If we're already loading or a retry is pending, skip auto-refresh
  std::time_t last_attempt_time_snapshot = 0;
  {
    const std::lock_guard<Mutex> lock(mutex);
    if (loading || retry_pending) {
      LogDebug("NOTAM: Auto-refresh skipped - loading={} retry_pending={}",
               loading, retry_pending);
      return;
    }
    last_attempt_time_snapshot = last_attempt_time;
  }

  GeoPoint last_loc = GetLastUpdateLocation();
  std::time_t last_time = GetLastUpdateTime();
  std::time_t now = std::time(nullptr);
  
  // Check if time interval has elapsed since last successful fetch
  bool time_expired = (last_time == 0) || 
                      (now - last_time >= static_cast<std::time_t>(
                                          settings.refresh_interval_min * 60));

  const unsigned last_radius_km = GetLastUpdateRadius();
  const bool radius_changed =
    (last_radius_km > 0) && (last_radius_km != settings.radius_km);
  
  // Also check if enough time has passed since last attempt (even if it
  // failed).
  // This prevents rapid retries when there's no network connection
  bool enough_time_since_attempt =
      (now - last_attempt_time_snapshot >=
       MIN_NOTAM_REFRESH_ATTEMPT_INTERVAL_SECONDS);
  
  // Check if location moved outside half the radius
  bool location_changed = last_loc.IsValid() &&
                          current_location.Distance(last_loc) >
                            (settings.radius_km * 1000.0 / 2.0);
  
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

  // Check if settings are enabled
  if (!settings.enabled) {
    return;
  }

  // Check if already loading
  {
    const std::lock_guard<Mutex> lock(mutex);
    if (loading) {
      return; // Already loading, skip this request
    }
    loading = true;
    retry_pending = false;
    current_location = location;
    last_attempt_time = std::time(nullptr);  // Record attempt time
  }
  
  // Log only when we actually start a fetch
  LogFormat("NOTAM: Auto-refresh starting");
  
  // Cancel any pending retry since we're starting a new attempt
  Cancel();

  // Start async loading
  load_task.Start(LoadNOTAMsInternal(location), 
                  BIND_THIS_METHOD(OnLoadComplete));
}

void
NOTAMGlue::LoadNOTAMs(const GeoPoint &location,
                      OperationEnvironment &operation)
{
  (void)operation; // Suppress unused parameter warning
  
  // Check if NOTAMs are enabled
  if (!settings.enabled) {
    return;
  }
  
  // Check if location is valid
  if (!location.IsValid()) {
    return;
  }
  
  // Check if already loading
  {
    const std::lock_guard lock(mutex);
    if (loading) {
      return;
    }
    loading = true;
    retry_pending = false;
    current_location = location;
  }
  
  // Cancel any pending retry since we're starting a new attempt
  Cancel();
  
  // Start async loading
  load_task.Start(LoadNOTAMsInternal(location), 
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
}

// Duplicate method removed

Co::InvokeTask
NOTAMGlue::LoadNOTAMsInternal(GeoPoint location)
{
  LogFormat("NOTAM: Starting LoadNOTAMsInternal for location %.6f,%.6f", 
            location.latitude.Degrees(), location.longitude.Degrees());

  try {
    // First, try to load from cache if not expired
    LogFormat("NOTAM: Checking cache expiration status");
    if (!IsCacheExpired()) {
      LogFormat("NOTAM: Cache is still valid, attempting to load from cache");
      std::vector<NOTAMStruct> cached_notams;
      GeoPoint cache_location = GeoPoint::Invalid();
      unsigned cache_radius_km = 0;
      boost::json::value cached_api;
      if (LoadNOTAMsFromFile(cached_notams, &cache_location, &cache_radius_km,
                             &cached_api)) {
        const unsigned count = static_cast<unsigned>(cached_notams.size());
        LogFormat("NOTAM: Successfully loaded %u NOTAMs from cache", count);

        const GeoPoint known_location =
          cache_location.IsValid() ? cache_location : location;
        const unsigned known_radius_km =
          cache_radius_km > 0 ? cache_radius_km : settings.radius_km;

        // Store cached results
        {
          const std::lock_guard<Mutex> lock(mutex);
          auto *impl = current_notams_impl.get();
          PopulateImplFromCache(*impl, std::move(cached_notams),
                                known_location, known_radius_km,
                                std::move(cached_api));
        }
        LogFormat("NOTAM: Using cached data, fetch complete");
        co_return; // Use cached data, no need to fetch
      } else {
        LogFormat("NOTAM: Failed to load from cache file, will fetch fresh "
                  "data");
      }
    } else {
      LogFormat("NOTAM: Cache is expired or doesn't exist, fetching fresh "
                "data");
    }
    
    // Cache is expired or doesn't exist, fetch fresh data from API
    LogFormat("NOTAM: Starting API fetch for radius %u km",
              settings.radius_km);

    // Use a null progress listener
    NullOperationEnvironment progress;

    NOTAMClient::KnownMap known_copy;
    {
      const std::lock_guard<Mutex> lock(mutex);
      auto *impl = current_notams_impl.get();
      if (CanUseDelta(*impl, location, settings.radius_km))
        known_copy = impl->known;
    }

    if (!known_copy.empty()) {
      try {
        LogFormat("NOTAM: Attempting delta fetch with %u known IDs",
                  static_cast<unsigned>(known_copy.size()));
        auto delta_response =
          co_await NOTAMClient::FetchNOTAMsResponse(curl, settings, location,
                                                    progress, &known_copy);

        if (delta_response.is_delta) {
          boost::json::value delta_json;
          if (!ParseJsonValue(delta_response.raw_json, delta_json)) {
            LogFormat("NOTAM: Delta response JSON parse failed, falling back");
          } else {
            boost::json::value api_snapshot;
            unsigned total = 0;
            bool applied = false;
            {
              const std::lock_guard<Mutex> lock(mutex);
              auto *impl = current_notams_impl.get();
              if (impl->cached_api_valid &&
                  ApplyDeltaToApi(impl->cached_api, delta_json,
                                  delta_response.removed_ids)) {
                ApplyDeltaUpdates(impl->current_notams, delta_response.notams,
                                  delta_response.removed_ids);
                impl->known = BuildKnownMap(impl->current_notams);
                impl->known_location = location;
                impl->known_radius_km = settings.radius_km;
                impl->last_update.timestamp = std::time(nullptr);
                impl->last_update.location = location;
                impl->last_update.radius_km = settings.radius_km;
                impl->last_update.valid = true;
                impl->last_update_cached = true;
                api_snapshot = impl->cached_api;
                total = static_cast<unsigned>(impl->current_notams.size());
                applied = true;
              }
            }

            if (applied) {
              SaveNOTAMsToFile(api_snapshot, location);

              LogFormat("NOTAM: Delta applied (updates=%u, removed=%u, "
                        "total=%u)",
                        static_cast<unsigned>(delta_response.notams.size()),
                        static_cast<unsigned>(
                          delta_response.removed_ids.size()),
                        total);

              StaticString<100> msg;
              msg.Format(_("Loaded %u NOTAMs"),
                         total);
              Message::AddMessage(msg.c_str());

              co_return;
            }

            LogFormat("NOTAM: Delta merge failed, falling back to full fetch");
          }
        } else {
          boost::json::value api_value;
          if (ParseJsonValue(delta_response.raw_json, api_value) &&
              IsApiResponseValid(api_value)) {
            unsigned total = 0;
            {
              const std::lock_guard<Mutex> lock(mutex);
              auto *impl = current_notams_impl.get();
              impl->current_notams = std::move(delta_response.notams);
              impl->known = BuildKnownMap(impl->current_notams);
              impl->known_location = location;
              impl->known_radius_km = settings.radius_km;
              impl->cached_api = api_value;
              impl->cached_api_valid = true;
              impl->last_update.timestamp = std::time(nullptr);
              impl->last_update.location = location;
              impl->last_update.radius_km = settings.radius_km;
              impl->last_update.valid = true;
              impl->last_update_cached = true;
              total = static_cast<unsigned>(impl->current_notams.size());
            }

            SaveNOTAMsToFile(api_value, location);

            LogFormat("NOTAM: Full response accepted (total=%u)",
                      total);

            StaticString<100> msg;
            msg.Format(_("Loaded %u NOTAMs"),
                       total);
            Message::AddMessage(msg.c_str());

            co_return;
          }

          LogFormat("NOTAM: Delta response missing full API payload");
        }

        LogFormat("NOTAM: Delta not applied, falling back to full fetch");
        {
          const std::lock_guard<Mutex> lock(mutex);
          auto *impl = current_notams_impl.get();
          impl->known.clear();
          impl->known_location = GeoPoint::Invalid();
          impl->known_radius_km = 0;
        }
      } catch (const std::exception &e) {
        LogFormat("NOTAM: Delta fetch failed: %s", e.what());
        {
          const std::lock_guard<Mutex> lock(mutex);
          auto *impl = current_notams_impl.get();
          impl->known.clear();
          impl->known_location = GeoPoint::Invalid();
          impl->known_radius_km = 0;
        }
      }
    }

    LogFormat("NOTAM: Performing full fetch");
    auto full_response =
      co_await NOTAMClient::FetchNOTAMsResponse(curl, settings, location,
                                                progress, nullptr);
    boost::json::value api_value;
    const bool api_valid =
      ParseJsonValue(full_response.raw_json, api_value) &&
      IsApiResponseValid(api_value);
    auto notams = std::move(full_response.notams);
    const unsigned count = static_cast<unsigned>(notams.size());
    LogFormat("NOTAM: Parsed %u NOTAMs from response", count);

    {
      const std::lock_guard<Mutex> lock(mutex);
      auto *impl = current_notams_impl.get();
      impl->current_notams = std::move(notams);
      impl->known = BuildKnownMap(impl->current_notams);
      impl->known_location = location;
      impl->known_radius_km = settings.radius_km;
      if (api_valid) {
        impl->cached_api = api_value;
        impl->cached_api_valid = true;
      } else {
        impl->cached_api = boost::json::value();
        impl->cached_api_valid = false;
      }
      impl->last_update.timestamp = std::time(nullptr);
      impl->last_update.location = location;
      impl->last_update.radius_km = settings.radius_km;
      impl->last_update.valid = true;
      impl->last_update_cached = true;
    }

    if (api_valid) {
      SaveNOTAMsToFile(api_value, location);
      LogFormat("NOTAM: Cache file saved successfully");
    } else {
      LogFormat("NOTAM: Skipping cache save (invalid API payload)");
    }

    LogFormat("NOTAM: Successfully completed fetch with %u NOTAMs", count);

    StaticString<100> msg;
    msg.Format(_("Loaded %u NOTAMs"), count);
    Message::AddMessage(msg.c_str());
    
  } catch (const std::exception &e) {
    LogFormat("NOTAM: Error during fetch: %s", e.what());
    
    // Keep existing NOTAMs on fetch failure - stale data is better than no
    // data.
    LogFormat("NOTAM: Keeping existing NOTAMs after fetch error");
    
    // Notify user of error
    Message::AddMessage(_("Failed to load NOTAMs"));
  }
}

void
NOTAMGlue::OnLoadComplete(std::exception_ptr error) noexcept
{
  // Reset loading flag
  bool schedule_retry = false;
  {
    const std::lock_guard<Mutex> lock(mutex);
    loading = false;
    if (error) {
      retry_pending = true;
      schedule_retry = true;
    } else {
      retry_pending = false;
    }
  }
  
  if (schedule_retry) {
    // Failed - schedule retry with fixed 30-second delay
    LogFormat("NOTAM: Fetch failed, scheduling retry in 30 seconds");
    Trigger();
  } else {
    // Success - cancel any pending retry
    Cancel();
    
    // Update the airspace database
    if (data_components && data_components->airspaces) {
      try {
        LogFormat("NOTAM: Updating airspace database with loaded NOTAMs");
        UpdateAirspaces(*data_components->airspaces);
        LogFormat("NOTAM: Airspace database updated successfully");
      } catch (const std::exception &e) {
        LogFormat("NOTAM: Error updating airspace database: %s", e.what());
      }
    }
    
    // Notify listeners that NOTAMs have been updated
    NotifyListeners();
  }
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
  if (it != listeners.end()) {
    listeners.erase(it);
  }
}

void
NOTAMGlue::NotifyListeners() noexcept
{
  std::vector<NOTAMListener *> listeners_copy;
  {
    const std::lock_guard<Mutex> lock(mutex);
    listeners_copy = listeners;
  }
  
  for (auto *listener : listeners_copy) {
    listener->OnNOTAMsUpdated();
  }
}

void
NOTAMGlue::Run()
{
  LogFormat("NOTAM: Retry timer fired, attempting fetch again");
  
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
  
  // Log the cache file path for debugging
  auto file_path = GetNOTAMCacheFilePath();
  LogFormat("NOTAM: Attempting to load cache from: %s",
            file_path.ToUTF8().c_str());
  
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
        
    LogFormat("NOTAM: Loaded %u NOTAMs from cache", count);
    return count;
  }
  
  LogFormat("NOTAM: No cached NOTAMs found at: %s",
            file_path.ToUTF8().c_str());
  return 0;
}

bool
NOTAMGlue::LoadCachedNOTAMsAndUpdate(Airspaces &airspaces)
{
  if (!settings.enabled) {
    LogDebug("NOTAM: Startup cache load skipped - disabled in settings");
    return false;
  }

  std::vector<NOTAMStruct> cached_notams;
  GeoPoint cache_location = GeoPoint::Invalid();
  unsigned cache_radius_km = 0;
  boost::json::value cached_api;
  if (!LoadNOTAMsFromFile(cached_notams, &cache_location, &cache_radius_km,
                          &cached_api)) {
    LogFormat("NOTAM: No cached NOTAMs to apply");
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

  LogFormat("NOTAM: Loaded %u NOTAMs from cache for startup", count);

  UpdateAirspaces(airspaces);
  NotifyListeners();

  return true;
}

void
NOTAMGlue::InvalidateCache()
{
  auto file_path = GetNOTAMCacheFilePath();
  
  if (File::Exists(file_path)) {
    LogFormat("NOTAM: Invalidating cache file: %s",
              file_path.ToUTF8().c_str());
    File::Delete(file_path);
    LogFormat("NOTAM: Cache invalidated successfully");
  } else {
    LogFormat("NOTAM: No cache file to invalidate at: %s",
              file_path.ToUTF8().c_str());
  }

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
  const std::lock_guard<Mutex> lock(mutex);
  auto *impl = current_notams_impl.get();
  
  unsigned count = 0;
  for (const auto &notam : impl->current_notams) {
    if (NOTAMFilter::ShouldDisplay(notam, settings, true)) {
      count++;
    }
  }
  return count;
}

NOTAMGlue::FilterStats
NOTAMGlue::GetFilterStats() const
{
  const std::lock_guard<Mutex> lock(mutex);
  auto *impl = current_notams_impl.get();
  
  FilterStats stats;
  stats.total = static_cast<unsigned>(impl->current_notams.size());
  stats.filtered_by_ifr = 0;
  stats.filtered_by_time = 0;
  stats.filtered_by_qcode = 0;
  stats.filtered_by_radius = 0;
  stats.final_count = 0;

  WideToUTF8Converter hidden_conv(settings.hidden_qcodes.c_str());
  const char *hidden_list = hidden_conv.IsValid() ? hidden_conv.c_str() : "";
  
  for (const auto &notam : impl->current_notams) {
    // Count how many fail IFR filter (independently)
    if (!settings.show_ifr && !notam.traffic.empty() && notam.traffic == "I") {
      stats.filtered_by_ifr++;
    }
    
    // Count how many fail time filter (independently)
    if (settings.show_only_effective) {
      auto now = std::chrono::system_clock::now();
      if (now < notam.start_time || now > notam.end_time) {
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
    if (NOTAMFilter::ShouldDisplay(notam, settings, true)) {
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
    if (notam.number == number) {
      return notam; // Return a copy while holding the lock
    }
  }
  
  return std::nullopt;
}

void
NOTAMGlue::UpdateAirspaces(Airspaces &airspaces)
{
  const std::lock_guard<Mutex> lock(mutex);
  auto *impl = current_notams_impl.get();
  
  LogFormat("NOTAM: UpdateAirspaces - converting %u NOTAMs to airspaces", 
            static_cast<unsigned>(impl->current_notams.size()));

  // Save all non-NOTAM airspaces
  // ToDo: Optimize by removing only existing NOTAM airspaces
  std::vector<AirspacePtr> saved_airspaces;
  for (const auto &airspace : airspaces.QueryAll()) {
    const auto &current = airspace.GetAirspace();
    if (current.GetClassOrType() != AirspaceClass::NOTAM) {
      saved_airspaces.push_back(airspace.GetAirspacePtr());
    }
  }
  
  LogFormat("NOTAM: Saved %u non-NOTAM airspaces", 
            static_cast<unsigned>(saved_airspaces.size()));
  
  // Clear all airspaces
  airspaces.Clear();
  
  // Restore non-NOTAM airspaces
  for (const auto &airspace : saved_airspaces) {
    airspaces.Add(airspace);
  }
  
  LogFormat("NOTAM: Restored %u non-NOTAM airspaces", 
            static_cast<unsigned>(saved_airspaces.size()));
  
  // Add filtered NOTAM airspaces
  unsigned added_count = 0;
  unsigned filtered_count = 0;
  
  for (const auto &notam : impl->current_notams) {
    try {
      // Apply filtering
      if (!NOTAMFilter::ShouldDisplay(notam, settings, true)) {
        filtered_count++;
        continue;
      }
      
      // Create airspace based on NOTAM geometry
      AirspacePtr airspace_ptr;
      
      switch (notam.geometry.type) {
        case NOTAM::NOTAMGeometry::Type::CIRCLE: {
          auto circle = std::make_shared<AirspaceCircle>(
            GeoPoint{Angle::Degrees(notam.geometry.center.longitude), 
                     Angle::Degrees(notam.geometry.center.latitude)},
            notam.geometry.radius_meters);
          airspace_ptr = circle;
          break;
        }
        
        case NOTAM::NOTAMGeometry::Type::POLYGON: {
          // Build vector of GeoPoints for the polygon
          std::vector<GeoPoint> polygon_points;
          for (const auto &point : notam.geometry.polygon_points) {
            polygon_points.emplace_back(Angle::Degrees(point.longitude), 
                                        Angle::Degrees(point.latitude));
          }
          
          if (!polygon_points.empty()) {
            auto polygon = std::make_shared<AirspacePolygon>(polygon_points);
            airspace_ptr = polygon;
          }
          break;
        }
        
        case NOTAM::NOTAMGeometry::Type::POINT:
        default: {
          // For points, create a small circle (1000m radius)
          auto circle = std::make_shared<AirspaceCircle>(
            GeoPoint{Angle::Degrees(notam.geometry.center.longitude), 
                     Angle::Degrees(notam.geometry.center.latitude)},
            1000.0); // 1km radius
          airspace_ptr = circle;
          break;
        }
      }
      
      if (airspace_ptr) {
        // Create the airspace main name, truncate if too long
        std::string main_text = notam.text;
        if (main_text.empty()) {
          main_text = "NOTAM";
        }
        if (main_text.length() > NOTAM_TEXT_TRUNCATE_LENGTH) {
          main_text =
            main_text.substr(0, NOTAM_TEXT_TRUNCATE_LENGTH - 3) + "...";
        }
        
        // Convert UTF-8 strings to TCHAR (wchar_t on Windows, char on Unix)
        UTF8ToWideConverter main_text_conv(main_text.c_str());
        UTF8ToWideConverter number_conv(notam.number.c_str());
        
        // Use converted strings, with fallback to empty if conversion failed
        tstring notam_name = main_text_conv.IsValid()
          ? tstring(main_text_conv.c_str())
          : _T("NOTAM");
        tstring notam_station = number_conv.IsValid()
          ? tstring(number_conv.c_str())
          : _T("");
        
        // Use the parsed AirspaceAltitude objects directly
        AirspaceAltitude base = notam.lower_altitude;
        AirspaceAltitude top = notam.upper_altitude;
        
        const auto is_invalid_altitude = [](const AirspaceAltitude &altitude) {
          return altitude.reference != AltitudeReference::AGL &&
                 altitude.reference != AltitudeReference::MSL &&
                 altitude.reference != AltitudeReference::STD;
        };

        const auto is_missing_msl = [](const AirspaceAltitude &altitude) {
          return altitude.reference == AltitudeReference::MSL &&
                 altitude.altitude == MISSING_MSL_ALTITUDE;
        };

        // Set reasonable defaults if altitudes are invalid
        if (is_invalid_altitude(base) || is_missing_msl(base)) {
          // Uninitialized or invalid - set to ground level
          base.reference = AltitudeReference::AGL;
          base.altitude_above_terrain = 0;
          base.altitude = 0;
          base.flight_level = 0;
        }
        
        if (is_invalid_altitude(top) || is_missing_msl(top)) {
          // Uninitialized or invalid - set to high MSL altitude
          top.reference = AltitudeReference::MSL;
          top.altitude = 9999;
          top.altitude_above_terrain = 0;
          top.flight_level = 0;
        }
        
        // Set airspace properties using SetProperties method
        airspace_ptr->SetProperties(
          std::move(notam_name),        // name: NOTAM detailed text
          std::move(notam_station),     // station_name: short identifier  
          TransponderCode::Null(), // transponder_code
          AirspaceClass::NOTAM, // class
          AirspaceClass::NOTAM, // type
          base,
          top
        );
        
        // Add to airspace database
        airspaces.Add(airspace_ptr);
        added_count++;
        
        LogDebug("NOTAM: Added airspace for NOTAM '{}' (type={}, "
                 "radius={:.0f}m)",
                 notam.number.c_str(), static_cast<int>(notam.geometry.type),
                 notam.geometry.radius_meters);
      }
    } catch (const std::exception &e) {
      LogFormat("NOTAM: Error creating airspace for NOTAM '%s': %s",
                notam.number.c_str(), e.what());
    }
  }
  
  // Optimize the airspace tree after adding NOTAMs
  airspaces.Optimise();
  
  LogFormat("NOTAM: UpdateAirspaces completed - added %u NOTAMs, "
            "filtered %u NOTAMs",
            added_count, filtered_count);
}

AllocatedPath
NOTAMGlue::GetNOTAMCacheFilePath() const
{
  // Use fixed filename in main XCSoarData directory
  // Keep a single cache file to simplify delta handling.
  return LocalPath(_T("notams.json"));
}

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

static void
ExtractCacheMetadata(const boost::json::object &obj,
                     CacheMetadata &meta) noexcept
{
  meta = CacheMetadata{};

  double timestamp = 0;
  if (auto it = obj.find("xcsoar_timestamp");
      it != obj.end() && GetJsonNumber(it->value(), timestamp)) {
    meta.timestamp = static_cast<std::time_t>(timestamp);
    meta.valid = true;
  }

  double lat = 0;
  double lon = 0;
  if (auto it_lat = obj.find("xcsoar_location_lat");
      it_lat != obj.end() && GetJsonNumber(it_lat->value(), lat)) {
    if (auto it_lon = obj.find("xcsoar_location_lon");
        it_lon != obj.end() && GetJsonNumber(it_lon->value(), lon)) {
      meta.location = GeoPoint(Angle::Degrees(lon), Angle::Degrees(lat));
      meta.valid = true;
    }
  }

  double radius = 0;
  if (auto it_radius = obj.find("xcsoar_radius_km");
      it_radius != obj.end() && GetJsonNumber(it_radius->value(), radius)) {
    meta.radius_km = radius >= 0
      ? static_cast<unsigned>(radius)
      : 0u;
    meta.valid = true;
  }
}

static void
ParseCacheMetadata(const boost::json::object &obj,
                   GeoPoint *location, unsigned *radius_km)
{
  CacheMetadata meta;
  ExtractCacheMetadata(obj, meta);

  if (location != nullptr)
    *location = meta.location;

  if (radius_km != nullptr)
    *radius_km = meta.radius_km;
}

static bool
LoadCacheJsonValue(const AllocatedPath &file_path, boost::json::value &root)
{
  try {
    FileReader file(Path(file_path.c_str()));

    std::string json_content;
    char buffer[4096];
    while (true) {
      size_t bytes_read = file.Read(std::as_writable_bytes(std::span{buffer}));
      if (bytes_read == 0)
        break;
      json_content.append(buffer, bytes_read);
    }

    if (json_content.empty())
      return false;

    boost::system::error_code ec;
    root = boost::json::parse(json_content, ec);
    return !ec;
  } catch (const std::exception &) {
    return false;
  }
}

static bool
LoadCacheMetadataFromFile(const AllocatedPath &file_path,
                          CacheMetadata &meta)
{
  boost::json::value root;
  if (!LoadCacheJsonValue(file_path, root) || !root.is_object()) {
    meta = CacheMetadata{};
    return false;
  }

  ExtractCacheMetadata(root.as_object(), meta);
  return meta.valid;
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

  CacheMetadata meta;
  LoadCacheMetadataFromFile(file_path, meta);

  {
    const std::lock_guard<Mutex> lock(mutex);
    impl.last_update = meta;
    impl.last_update_cached = true;
  }

  return meta;
}

void
NOTAMGlue::SaveNOTAMsToFile(const boost::json::value &api_response,
                            const GeoPoint &location) const
{
  try {
    if (!IsApiResponseValid(api_response)) {
      LogFormat("NOTAM: Skipping cache save (invalid API response)");
      return;
    }

    auto file_path = GetNOTAMCacheFilePath();
    WideToUTF8Converter path_conv(file_path.c_str());
    const char *path_utf8 = path_conv.IsValid() ? path_conv.c_str() : "";
    LogFormat("NOTAM: Saving NOTAM cache: %s", path_utf8);

    boost::json::object wrapper;
    const auto now = std::time(nullptr);
    wrapper["xcsoar_timestamp"] = static_cast<std::int64_t>(now);
    wrapper["xcsoar_location_lat"] = location.latitude.Degrees();
    wrapper["xcsoar_location_lon"] = location.longitude.Degrees();
    wrapper["xcsoar_radius_km"] = settings.radius_km;
    wrapper["xcsoar_refresh_interval_min"] = settings.refresh_interval_min;
    wrapper["api"] = api_response;

    std::string json_with_metadata =
      SerializeJsonValueReadable(boost::json::value(std::move(wrapper)));
    
    // Write to file
    FileOutputStream file(Path(file_path.c_str()));
    file.Write(std::as_bytes(std::span{json_with_metadata}));
    file.Commit();
    
    LogFormat("NOTAM: Saved %u bytes of NOTAM cache",
              static_cast<unsigned>(json_with_metadata.size()));
  } catch (const std::exception &e) {
    LogFormat("NOTAM: Error saving NOTAM cache to file: %s", e.what());
  }
}

bool
NOTAMGlue::LoadNOTAMsFromFile(std::vector<NOTAMStruct> &notams,
                              GeoPoint *location,
                              unsigned *radius_km,
                              boost::json::value *api_response) const
{
  try {
    auto file_path = GetNOTAMCacheFilePath();
    WideToUTF8Converter path_conv(file_path.c_str());
    const char *path_utf8 = path_conv.IsValid() ? path_conv.c_str() : "";
    LogFormat("NOTAM: LoadNOTAMsFromFile attempting to load: %s",
              path_utf8);

    std::string json_content;
    try {
      FileReader file(Path(file_path.c_str()));

      // Read the entire file to get the JSON data
      char buffer[4096];
      while (true) {
        size_t bytes_read =
          file.Read(std::as_writable_bytes(std::span{buffer}));
        if (bytes_read == 0) break;
        json_content.append(buffer, bytes_read);
      }
    } catch (const std::exception &e) {
      LogFormat("NOTAM: LoadNOTAMsFromFile failed to open file: %s - %s",
                path_utf8, e.what());
      return false; // File doesn't exist or can't be opened
    }

    LogFormat("NOTAM: LoadNOTAMsFromFile read %u bytes from file",
              static_cast<unsigned>(json_content.size()));

    if (json_content.empty()) {
      LogFormat("NOTAM: LoadNOTAMsFromFile file is empty");
      return false;
    }

    if (location != nullptr)
      *location = GeoPoint::Invalid();
    if (radius_km != nullptr)
      *radius_km = 0;
    if (api_response != nullptr)
      *api_response = boost::json::value();

    boost::system::error_code ec;
    boost::json::value root = boost::json::parse(json_content, ec);
    if (ec) {
      LogFormat("NOTAM: LoadNOTAMsFromFile JSON parse error: %s",
                ec.message().c_str());
      return false;
    }

    notams.clear();

    if (root.is_object()) {
      const auto &obj = root.as_object();
      ParseCacheMetadata(obj, location, radius_km);

      if (auto it = obj.find("api"); it != obj.end()) {
        const auto &api_value = it->value();
        if (api_response != nullptr)
          *api_response = api_value;
        if (!IsApiResponseValid(api_value)) {
          LogFormat("NOTAM: LoadNOTAMsFromFile invalid api payload");
          return false;
        }
        if (!ParseNOTAMsFromApiValue(api_value, notams, "api"))
          return false;

        LogFormat("NOTAM: LoadNOTAMsFromFile parsed %u NOTAMs from api",
                  static_cast<unsigned>(notams.size()));
        return true;
      }
    }

    LogFormat("NOTAM: LoadNOTAMsFromFile missing api payload");
    return false;
  } catch (const std::exception &e) {
    LogFormat("NOTAM: LoadNOTAMsFromFile exception: %s", e.what());
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
    const auto file_path = GetNOTAMCacheFilePath();
    const auto meta =
      GetCachedMetadata(*current_notams_impl, mutex, file_path);

    if (meta.timestamp <= 0)
      return true;

    if (!meta.location.IsValid())
      return true;

    std::time_t current_time = std::time(nullptr);
    std::time_t max_age_seconds = settings.refresh_interval_min * 60;

    if ((current_time - meta.timestamp) > max_age_seconds)
      return true;

    if (meta.radius_km > 0 && meta.radius_km != settings.radius_km)
      return true;

    GeoPoint location_copy;
    {
      const std::lock_guard<Mutex> lock(mutex);
      location_copy = current_location;
    }

    if (location_copy.IsValid()) {
      double distance_m = location_copy.Distance(meta.location);
      double radius_m = settings.radius_km * 1000.0;
      if (distance_m > radius_m)
        return true;
    }

    return false;
  } catch (const std::exception &e) {
    return true; // Error means treat as expired
  }
}

#endif // HAVE_HTTP
