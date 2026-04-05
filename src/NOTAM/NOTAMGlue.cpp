// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The XCSoar Project

#include "NOTAMGlue.hpp"
#include "AirspaceSync.hpp"
#include "NOTAMCache.hpp"
#include "NOTAM.hpp"
#include "Client.hpp"
#include "Delta.hpp"
#include "Filter.hpp"
#include "Sync.hpp"
#include "Components.hpp"
#include "DataComponents.hpp"
#include "Airspace/AirspaceGlue.hpp"
#include "Airspace/Airspaces.hpp"
#include "Airspace/AirspaceClass.hpp"
#include "Engine/Airspace/AbstractAirspace.hpp"
#include "Protection.hpp"
#include "Operation/Operation.hpp"
#include "Operation/ProgressListener.hpp"
#include "system/Path.hpp"
#include "LogFile.hpp"
#include "util/SpanCast.hxx"
#include "thread/Mutex.hxx"
#include "co/Task.hxx"
#include "co/InvokeTask.hxx"
#include "lib/curl/Global.hxx"
#include "time/Convert.hxx"

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
  std::unordered_set<const AbstractAirspace *> injected_airspaces;
  GeoPoint known_location = GeoPoint::Invalid();
  unsigned known_radius_km = 0;
  boost::json::value cached_api;
  bool cached_api_valid = false;
  CacheMetadata last_update;
  bool last_update_cached = false;
};

static void
PopulateImplFromCache(NOTAMImpl &impl,
                      std::vector<NOTAMStruct> &&cached_notams,
                      const GeoPoint &known_location,
                      unsigned known_radius_km,
                      boost::json::value cached_api)
{
  const bool cached_api_valid = NOTAMDelta::IsApiResponseValid(cached_api);
  impl.current_notams = std::move(cached_notams);
  impl.known = NOTAMDelta::BuildKnownMap(impl.current_notams);
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
  const bool api_base_url_changed =
    settings.api_base_url != _settings.api_base_url;
  const bool request_settings_changed =
    settings.enabled != _settings.enabled ||
    settings.radius_km != _settings.radius_km ||
    api_base_url_changed;

  settings = _settings;

  if (api_base_url_changed) {
    auto *impl = current_notams_impl.get();
    impl->known.clear();
    impl->known_location = GeoPoint::Invalid();
    impl->known_radius_km = 0;
    impl->cached_api = boost::json::value();
    impl->cached_api_valid = false;
  }

  if (request_settings_changed)
    ++mutation_generation;
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
  try {
    load_task.Start(LoadNOTAMsInternal(location, false),
                    BIND_THIS_METHOD(OnLoadComplete));
  } catch (const std::exception &e) {
    {
      const std::lock_guard<Mutex> lock(mutex);
      loading = false;
      retry_pending = false;
      last_load_committed = false;
      manual_refresh_requested = false;
    }
    LogFmt("NOTAM: Failed to start auto-refresh: {}", e.what());
  } catch (...) {
    {
      const std::lock_guard<Mutex> lock(mutex);
      loading = false;
      retry_pending = false;
      last_load_committed = false;
      manual_refresh_requested = false;
    }
    LogFmt("NOTAM: Failed to start auto-refresh");
  }
}

bool
NOTAMGlue::ForceUpdateLocation(const GeoPoint &location,
                               const bool invalidate_cache_state)
{
  if (!location.IsValid()) {
    const std::lock_guard<Mutex> lock(mutex);
    manual_refresh_requested = false;
    return false;
  }

  {
    const std::lock_guard<Mutex> lock(mutex);
    if (!settings.enabled) {
      manual_refresh_requested = false;
      return false;
    }

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

    if (loading) {
      force_refresh_pending = true;
      retry_pending = false;
      return true;
    }

    loading = true;
    retry_pending = false;
    last_load_committed = false;
  }

  LogFmt("NOTAM: Manual refresh starting");
  Cancel();
  try {
    load_task.Start(LoadNOTAMsInternal(location, true),
                    BIND_THIS_METHOD(OnLoadComplete));
  } catch (const std::exception &e) {
    {
      const std::lock_guard<Mutex> lock(mutex);
      loading = false;
      retry_pending = false;
      last_load_committed = false;
      manual_refresh_requested = false;
    }
    LogFmt("NOTAM: Failed to start manual refresh: {}", e.what());
    return false;
  } catch (...) {
    {
      const std::lock_guard<Mutex> lock(mutex);
      loading = false;
      retry_pending = false;
      last_load_committed = false;
      manual_refresh_requested = false;
    }
    LogFmt("NOTAM: Failed to start manual refresh");
    return false;
  }
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
  try {
    load_task.Start(LoadNOTAMsInternal(location, false),
                    BIND_THIS_METHOD(OnLoadComplete));
  } catch (const std::exception &e) {
    {
      const std::lock_guard<Mutex> lock(mutex);
      loading = false;
      retry_pending = false;
      last_load_committed = false;
      manual_refresh_requested = false;
    }
    LogFmt("NOTAM: Failed to start load: {}", e.what());
  } catch (...) {
    {
      const std::lock_guard<Mutex> lock(mutex);
      loading = false;
      retry_pending = false;
      last_load_committed = false;
      manual_refresh_requested = false;
    }
    LogFmt("NOTAM: Failed to start load");
  }
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

  const auto cache_path = GetNOTAMCacheFilePath();
  GeoPoint location_copy;
  NOTAMSync::CachedState sync_state;
  {
    const std::lock_guard<Mutex> lock(mutex);
    location_copy = current_location;
    auto *impl = current_notams_impl.get();
    sync_state.current_notams = impl->current_notams;
    sync_state.known = impl->known;
    sync_state.cached_api = impl->cached_api;
    sync_state.cached_api_valid = impl->cached_api_valid;
    sync_state.known_location = impl->known_location;
    sync_state.known_radius_km = impl->known_radius_km;
  }

  const auto meta =
    GetCachedMetadata(*current_notams_impl, mutex, cache_path);
  const bool cache_expired = NOTAMCache::IsExpired(meta, settings_snapshot,
                                                   location_copy);

  NullOperationEnvironment progress;

  try {
    LogFmt("NOTAM: Checking cache expiration status");
    auto sync_result =
      co_await NOTAMSync::Fetch(curl, settings_snapshot, location,
                                force_network, cache_expired, cache_path,
                                progress, sync_state);

    if (!can_commit())
      co_return;

    const auto outcome = sync_result.outcome;
    const unsigned count =
      static_cast<unsigned>(sync_result.notams.size());
    boost::json::value api_snapshot;

    {
      const std::lock_guard<Mutex> lock(mutex);
      if (!settings.enabled || request_generation != mutation_generation)
        co_return;

      auto *impl = current_notams_impl.get();
      impl->current_notams = std::move(sync_result.notams);
      impl->known = NOTAMDelta::BuildKnownMap(impl->current_notams);
      impl->known_location = sync_result.known_location;
      impl->known_radius_km = sync_result.known_radius_km;
      impl->cached_api = std::move(sync_result.document);
      impl->cached_api_valid =
        NOTAMDelta::IsApiResponseValid(impl->cached_api);
      impl->last_update.timestamp = std::time(nullptr);
      impl->last_update.location = sync_result.known_location;
      impl->last_update.radius_km = sync_result.known_radius_km;
      impl->last_update.valid = true;
      impl->last_update_cached = true;
      last_load_committed = true;

      if (outcome != NOTAMSync::Outcome::DiskCache)
        api_snapshot = impl->cached_api;
    }

    if (outcome != NOTAMSync::Outcome::DiskCache)
      SaveNOTAMsToFile(api_snapshot, location, request_generation);

    if (outcome == NOTAMSync::Outcome::DiskCache)
      LogFmt("NOTAM: Using cached data, fetch complete");
    else
      LogFmt("NOTAM: Successfully completed fetch with {} NOTAMs", count);

  } catch (const std::exception &e) {
    LogFmt("NOTAM: Error during fetch: {}", e.what());

    if (!can_commit()) {
      LogFmt("NOTAM: Ignoring stale fetch error");
      co_return;
    }
    
    // Keep existing NOTAMs on fetch failure - stale data is better than no
    // data.
    LogFmt("NOTAM: Keeping existing NOTAMs after fetch error");

    throw;
  }
}

void
NOTAMGlue::OnLoadComplete(std::exception_ptr error) noexcept
{
  {
    const std::lock_guard<Mutex> lock(mutex);
    load_complete_error = std::move(error);
    load_complete_pending = true;
  }

  load_complete_notify.SendNotification();
}

void
NOTAMGlue::OnLoadCompleteNotification() noexcept
{
  std::exception_ptr error;
  {
    const std::lock_guard<Mutex> lock(mutex);
    if (!load_complete_pending)
      return;

    error = std::move(load_complete_error);
    load_complete_error = {};
    load_complete_pending = false;
  }

  HandleLoadComplete(std::move(error));
}

void
NOTAMGlue::HandleLoadComplete(std::exception_ptr error) noexcept
{
  // Reset loading flag
  bool load_failed = false;
  bool load_committed = false;
  bool schedule_retry = false;
  bool start_queued_refresh = false;
  GeoPoint queued_location = GeoPoint::Invalid();
  {
    const std::lock_guard<Mutex> lock(mutex);
    loading = false;
    load_committed = last_load_committed;
    last_load_committed = false;
    if (force_refresh_pending) {
      force_refresh_pending = false;
      if (settings.enabled && current_location.IsValid()) {
        start_queued_refresh = true;
        queued_location = current_location;
        loading = true;
        retry_pending = false;
        last_load_committed = false;
        last_attempt_time = std::time(nullptr);
      }
    } else if (error) {
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

  const auto notify_load_complete =
    [this](NOTAMLoadNotification notification) {
      {
        const std::lock_guard<Mutex> lock(mutex);
        listener_load_complete_pending = notification;
        if (!notification.success)
          manual_refresh_requested = false;
      }
      listener_notify.SendNotification();
    };

  const auto apply_committed_load =
    [this, notify_load_complete, start_queued_refresh]() {
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
        notify_load_complete({});
        return;
      } catch (...) {
        LogFmt("NOTAM: Error updating airspace database");
        notify_load_complete({});
        return;
      }
    }

    // Notify listeners that NOTAMs have been updated
    NotifyListeners();

    NOTAMLoadNotification notification;
    notification.success = true;
    bool send_load_complete = false;
    {
      const std::lock_guard<Mutex> lock(mutex);
      if (!start_queued_refresh) {
        send_load_complete = true;
      }
      notification.loaded_count =
        static_cast<unsigned>(current_notams_impl->current_notams.size());
      if (!start_queued_refresh) {
        notification.show_loaded_message = manual_refresh_requested;
        manual_refresh_requested = false;
      }
    }

    if (send_load_complete)
      notify_load_complete(notification);
  };

  if (start_queued_refresh) {
    if (load_committed)
      apply_committed_load();
    else
      Cancel();

    LogFmt("NOTAM: Starting queued forced refresh");
    try {
      load_task.Start(LoadNOTAMsInternal(queued_location, true),
                      BIND_THIS_METHOD(OnLoadComplete));
    } catch (const std::exception &e) {
      {
        const std::lock_guard<Mutex> lock(mutex);
        loading = false;
        retry_pending = false;
        last_load_committed = false;
        manual_refresh_requested = false;
      }
      LogFmt("NOTAM: Failed to start queued refresh: {}", e.what());
    } catch (...) {
      {
        const std::lock_guard<Mutex> lock(mutex);
        loading = false;
        retry_pending = false;
        last_load_committed = false;
        manual_refresh_requested = false;
      }
      LogFmt("NOTAM: Failed to start queued refresh");
    }
    return;
  }

  if (load_failed) {
    bool show_error_message = false;
    {
      const std::lock_guard<Mutex> lock(mutex);
      if (!fetch_failure_notified) {
        fetch_failure_notified = true;
        show_error_message = true;
      }
    }

    NOTAMLoadNotification notification;
    notification.show_fetch_failure_message = show_error_message;
    notify_load_complete(notification);

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
    apply_committed_load();
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
  listeners.Add(&listener);
}

void
NOTAMGlue::RemoveListener(NOTAMListener &listener) noexcept
{
  listeners.Remove(&listener);
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
  std::optional<NOTAMLoadNotification> notify_load_complete;
  {
    const std::lock_guard<Mutex> lock(mutex);
    notify_updated = listener_update_pending;
    listener_update_pending = false;
    notify_load_complete = listener_load_complete_pending;
    listener_load_complete_pending.reset();
  }

  if (notify_updated)
    listeners.ForEach([](auto *listener) {
      listener->OnNOTAMsUpdated();
    });

  if (notify_load_complete.has_value())
    listeners.ForEach([notification = *notify_load_complete](auto *listener) {
      listener->OnNOTAMsLoadComplete(notification);
    });
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
  std::string cache_api_base_url;
  boost::json::value cached_api;
  
  LogFmt("NOTAM: Attempting to load NOTAM cache");
  
  // Try to load from cache file
  if (LoadNOTAMsFromFile(cached_notams, &cache_location, &cache_radius_km,
                         &cache_api_base_url, &cached_api)) {
    const auto settings_snapshot = GetSettingsSnapshot();
    if (cache_api_base_url != settings_snapshot.api_base_url.c_str()) {
      LogFmt("NOTAM: Cache load skipped - cached API endpoint differs");
      return 0;
    }

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
  std::string cache_api_base_url;
  boost::json::value cached_api;
  std::time_t cache_timestamp = 0;
  if (!LoadNOTAMsFromFile(cached_notams, &cache_location, &cache_radius_km,
                          &cache_api_base_url, &cached_api,
                          &cache_timestamp)) {
    LogFmt("NOTAM: No cached NOTAMs to apply");
    return false;
  }

  const auto max_age_seconds =
    static_cast<std::time_t>(settings_snapshot.refresh_interval_min) * 60;
  if (cache_timestamp <= 0) {
    LogFmt("NOTAM: Startup cache replay skipped - missing timestamp metadata");
    return false;
  }

  if (max_age_seconds > 0 &&
      std::time(nullptr) - cache_timestamp > max_age_seconds) {
    LogFmt("NOTAM: Startup cache replay skipped - cached data is expired");
    return false;
  }

  if (cache_api_base_url != settings_snapshot.api_base_url.c_str()) {
    LogFmt("NOTAM: Startup cache replay skipped - cached API endpoint "
           "differs");
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

  try {
    UpdateAirspaces(airspaces);
  } catch (const std::exception &e) {
    LogFmt("NOTAM: Error updating airspace database from startup cache: {}",
           e.what());
    InvalidateCache();
    if (current_location.IsValid())
      UpdateLocation(current_location);
    return false;
  } catch (...) {
    LogFmt("NOTAM: Error updating airspace database from startup cache");
    InvalidateCache();
    if (current_location.IsValid())
      UpdateLocation(current_location);
    return false;
  }

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
    force_refresh_pending = false;
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
  return NOTAMFilter::CountDisplayed(current_notams_impl->current_notams,
                                     settings, now);
}

FilterStats
NOTAMGlue::GetFilterStats() const
{
  const auto now = GetCurrentTimeUTCSnapshot();
  const std::lock_guard<Mutex> lock(mutex);
  return NOTAMFilter::ComputeStats(current_notams_impl->current_notams,
                                   settings, now);
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
  std::vector<NOTAMStruct> notams;
  std::unordered_set<const AbstractAirspace *> previous_injected;
  NOTAMSettings settings_snapshot;
  {
    const std::lock_guard<Mutex> lock(mutex);
    auto *impl = current_notams_impl.get();
    notams = impl->current_notams;
    previous_injected = impl->injected_airspaces;
    settings_snapshot = settings;
  }

  auto result = NOTAMAirspaceSync::Rebuild(airspaces, notams,
                                            settings_snapshot, now,
                                            previous_injected);

  const std::lock_guard<Mutex> lock(mutex);
  current_notams_impl->injected_airspaces.swap(result.injected);
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
  if (!NOTAMDelta::IsApiResponseValid(api_response)) {
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
                           settings_snapshot.api_base_url.c_str(),
                           validate_commit);
  } catch (const std::exception &e) {
    LogFmt("NOTAM: Error saving NOTAM cache to file: {}", e.what());
  }
}

bool
NOTAMGlue::LoadNOTAMsFromFile(std::vector<NOTAMStruct> &notams,
                               GeoPoint *location,
                               unsigned *radius_km,
                               std::string *api_base_url,
                               boost::json::value *api_response,
                               std::time_t *timestamp) const
{
  LogFmt("NOTAM: LoadNOTAMsFromFile attempting to load cache");

  NOTAMCache::NOTAMCacheBundle bundle;
  if (!NOTAMCache::LoadBundle(GetNOTAMCacheFilePath(), bundle))
    return false;

  if (location != nullptr)
    *location = bundle.meta.location;
  if (radius_km != nullptr)
    *radius_km = bundle.meta.radius_km;
  if (api_base_url != nullptr)
    *api_base_url = bundle.meta.api_base_url;
  if (timestamp != nullptr)
    *timestamp = bundle.meta.timestamp;
  if (api_response != nullptr)
    *api_response = std::move(bundle.api);

  notams = std::move(bundle.notams);
  return true;
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
  const auto settings_snapshot = GetSettingsSnapshot();
  const auto file_path = GetNOTAMCacheFilePath();
  const auto meta =
    GetCachedMetadata(*current_notams_impl, mutex, file_path);

  GeoPoint location_copy;
  {
    const std::lock_guard<Mutex> lock(mutex);
    location_copy = current_location;
  }

  return NOTAMCache::IsExpired(meta, settings_snapshot, location_copy);
}

#endif // HAVE_HTTP
