// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The XCSoar Project

#include "XCThermAPI.hpp"
#include "Weather/Settings.hpp"
#include "Weather/xctherm/XCThermCatalog.hpp"
#include "LocalPath.hpp"
#include "LogFile.hpp"
#include "io/FileOutputStream.hxx"
#include "io/FileReader.hxx"
#include "lib/fmt/ToBuffer.hxx"
#include "co/InvokeTask.hxx"
#include "lib/curl/Global.hxx"
#include "util/ReturnValue.hxx"
#include "net/client/xctherm/Http.hpp"
#include "net/http/Init.hpp"
#include "system/FileUtil.hpp"
#include "system/Path.hpp"

#include <algorithm>
#include <chrono>
#include <cstdio>
#include <cstring>
#include <ctime>
#include <set>
#include <span>
#include <string_view>
#include <vector>

#include <boost/json.hpp>

/* ------------------------------------------------------------------ */
/* Singleton                                                           */
/* ------------------------------------------------------------------ */

XCThermAPI &
XCThermAPI::Instance() {
  static XCThermAPI instance;
  return instance;
}

XCThermAPI::XCThermAPI() = default;
XCThermAPI::~XCThermAPI() = default;

void
XCThermAPI::SetCredentials(const std::string &email,
                           const std::string &password) noexcept
{
  auth.SetCredentials(email, password);
}

void
XCThermAPI::SetModel(const std::string &m) noexcept
{
  if (model != m) {
    model = m;
    available_parameters.clear();
    index_loaded = false;
  }
}

void
XCThermAPI::SetRegion(const unsigned region_model_id) noexcept
{
  SetModel(XCTherm::GetRegion(region_model_id).api_slug);
}

void
XCThermAPI::ApplySessionSettings(const XCThermSettings &settings) noexcept
{
  SetCredentials(settings.credentials.email.c_str(),
                 settings.credentials.password.c_str());
  SetRegion(settings.model);
}

void
XCThermAPI::PrepareSession(const XCThermSettings &settings) noexcept
{
  EnableDiskCache();
  ApplySessionSettings(settings);
}

/* ------------------------------------------------------------------ */
/* Index.json                                                          */
/* ------------------------------------------------------------------ */

/**
 * Map a (curl result, HTTP code) pair to an XCThermAPIError with a
 * user-readable message. Caller decides whether to retry or surface.
 */
static XCThermAPIError
MakeApiError(bool transfer_failed, long http_code, const std::string &context)
{
  if (transfer_failed) {
    std::string msg = "Network error (";
    msg += context;
    msg += ")";
    return XCThermAPIError(XCThermAPIError::Kind::NETWORK, 0,
                           std::move(msg));
  }

  XCThermAPIError::Kind kind;
  std::string msg;
  switch (http_code) {
  case 401:
    kind = XCThermAPIError::Kind::AUTH_FAILED;
    msg = "Authentication expired or invalid. "
          "Check XCTherm credentials in Config → System → Weather.";
    break;
  case 403:
    kind = XCThermAPIError::Kind::FORBIDDEN;
    msg = "Access denied by XCTherm server. "
          "Your account may not have access to this region.";
    break;
  case 404:
    kind = XCThermAPIError::Kind::NOT_FOUND;
    msg = "Forecast slot not available on server (" + context + ").";
    break;
  default:
    if (http_code >= 500 && http_code < 600) {
      kind = XCThermAPIError::Kind::SERVER_ERROR;
      msg = "XCTherm server error (HTTP " + std::to_string(http_code) +
            "). Will retry.";
    } else {
      kind = XCThermAPIError::Kind::OTHER_HTTP;
      msg = "Unexpected HTTP " + std::to_string(http_code) +
            " from XCTherm server.";
    }
    break;
  }
  msg += " [" + context + "]";
  return XCThermAPIError(kind, http_code, std::move(msg));
}

Co::Task<bool>
XCThermAPI::CoFetchIndex(CurlGlobal &curl)
{
  LogFmt("xctherm: fetching index for model='{}'", model);

  const auto url = FmtBuffer<256>(
    "https://tiles.xctherm.com/forecast/{}/index.json", model);

  Curl::CoResponse response;
  try {
    response = co_await XCTherm::Http::CoGet(curl, url.c_str());
  } catch (...) {
    LogFmt("xctherm: index fetch failed (network)");
    throw MakeApiError(true, 0, "index.json");
  }

  if (response.status != 200) {
    LogFmt("xctherm: index fetch failed http={}", response.status);
    throw MakeApiError(response.status == 0, (long)response.status,
                       "index.json");
  }

  LogFmt("xctherm: index fetched, {} bytes", response.body.size());
  co_return ParseIndex(response.body);
}

Co::Task<bool>
XCThermAPI::CoEnsureIndexLoaded(CurlGlobal &curl)
{
  if (IsIndexLoaded())
    co_return true;

  co_return co_await CoFetchIndex(curl);
}

bool
XCThermAPI::FetchIndex()
{
  if (Net::curl == nullptr)
    throw MakeApiError(true, 0, "index.json");

  ReturnValue<bool> parsed;
  const auto invoke = [&parsed](Co::Task<bool> task) -> Co::InvokeTask {
    parsed.Set(co_await task);
  };
  XCTherm::Http::RunSync(*Net::curl, invoke(CoFetchIndex(*Net::curl)));
  return std::move(parsed).Get();
}

/* ------------------------------------------------------------------ */
/* index.json parsing via boost::json                                  */
/* ------------------------------------------------------------------ */

/**
 * Extract a YYYYMMDD date and HH hour from an ISO-8601 "run" timestamp
 * like "2026-05-18T12:00:00+00:00". Conservative: returns false if the
 * shape doesn't match the expected length and structure.
 */
static bool
ParseRunTimestamp(std::string_view iso,
                  std::string &out_date,
                  std::string &out_hour) noexcept
{
  if (iso.size() < 13 || iso[4] != '-' || iso[7] != '-' ||
      iso[10] != 'T')
    return false;
  out_date.clear();
  out_date.append(iso.substr(0, 4));
  out_date.append(iso.substr(5, 2));
  out_date.append(iso.substr(8, 2));
  out_hour = iso.substr(11, 2);
  return true;
}

/**
 * Convenience: try to get a uint from a json value, defaulting if the
 * value is missing or of the wrong kind. Useful because the API may
 * send `"step": null` for partially-filled runs.
 */
static unsigned
TryGetUInt(const boost::json::object &obj, std::string_view key,
           unsigned default_val) noexcept
{
  auto it = obj.find(key);
  if (it == obj.end())
    return default_val;
  try {
    return (unsigned)it->value().to_number<int64_t>();
  } catch (...) {
    return default_val;
  }
}

bool
XCThermAPI::ParseIndex(const std::string &json) noexcept
{
  available_parameters.clear();
  index_loaded = false;

  boost::json::value root;
  try {
    root = boost::json::parse(json);
  } catch (const std::exception &e) {
    LogFmt("xctherm: index.json is not valid JSON: {}", e.what());
    return false;
  }

  if (!root.is_object()) {
    LogFmt("xctherm: index.json root is not an object");
    return false;
  }

  const auto &root_obj = root.as_object();
  auto it_params = root_obj.find("parameters");
  if (it_params == root_obj.end() || !it_params->value().is_object()) {
    LogFmt("xctherm: no 'parameters' object in index.json");
    return false;
  }

  for (const auto &param_kv : it_params->value().as_object()) {
    std::string_view param_name{param_kv.key()};
    /* We only consume "vertical_wind_*" — other params (CAPE, IR, …)
       exist in some forecasts and are silently ignored. */
    if (!param_name.starts_with("vertical_wind_"))
      continue;
    if (!param_kv.value().is_object())
      continue;

    const auto &param_obj = param_kv.value().as_object();
    auto it_slots = param_obj.find("slots");
    if (it_slots == param_obj.end() || !it_slots->value().is_array())
      continue;

    ParameterInfo info;
    info.name = std::string{param_name};

    for (const auto &slot_val : it_slots->value().as_array()) {
      if (!slot_val.is_object())
        continue;
      const auto &slot_obj = slot_val.as_object();

      auto it_run = slot_obj.find("run");
      if (it_run == slot_obj.end() || !it_run->value().is_string())
        continue;

      ForecastSlot slot;
      if (!ParseRunTimestamp(it_run->value().as_string().c_str(),
                             slot.run_date, slot.run_hour))
        continue;

      auto it_steps = slot_obj.find("steps");
      if (it_steps != slot_obj.end() && it_steps->value().is_object()) {
        const auto &steps_obj = it_steps->value().as_object();
        slot.step_min  = TryGetUInt(steps_obj, "min", 0);
        slot.step_max  = TryGetUInt(steps_obj, "max", 0);
        slot.step_step = TryGetUInt(steps_obj, "step", 1);
        if (slot.step_step == 0)
          slot.step_step = 1;
      }

      info.slots.push_back(std::move(slot));
    }

    if (!info.slots.empty()) {
      LogFmt("xctherm: param '{}' — {} slots",
             info.name, info.slots.size());
      available_parameters.push_back(std::move(info));
    }
  }

  index_loaded = !available_parameters.empty();
  LogFmt("xctherm: index parsed, {} vertical_wind parameters",
         available_parameters.size());
  return index_loaded;
}

/* ------------------------------------------------------------------ */
/* Available forecast hours                                            */
/* ------------------------------------------------------------------ */

std::vector<unsigned>
XCThermAPI::GetAvailableForecastHours(const std::string &parameter) const noexcept
{
  std::set<unsigned> hours;

  for (const auto &p : available_parameters) {
    if (p.name != parameter)
      continue;

    for (const auto &slot : p.slots) {
      unsigned run_h = (unsigned)std::atoi(slot.run_hour.c_str());
      for (unsigned s = slot.step_min; s <= slot.step_max; s += slot.step_step) {
        unsigned forecast_h = (run_h + s) % 24;
        hours.insert(forecast_h);
      }
    }
  }

  return {hours.begin(), hours.end()};
}

/* ------------------------------------------------------------------ */
/* Find best slot for an offset from current time                      */
/* ------------------------------------------------------------------ */

bool
XCThermAPI::FindSlotForOffset(const std::string &parameter,
                              unsigned current_utc_hour,
                              unsigned offset_hours,
                              std::string &out_date,
                              std::string &out_run_hour,
                              unsigned &out_step) const noexcept
{
  /*
   * We want: forecast for (current_utc + offset) hours from now.
   * For a run starting at run_h, the required step is:
   *   step = (current_utc - run_h) + offset
   * If run_h > current_utc (run from yesterday still valid), add 24:
   *   step = (current_utc + 24 - run_h) + offset
   *
   * We prefer the slot with the highest step (= most future) from
   * the latest available run.
   */
  for (const auto &p : available_parameters) {
    if (p.name != parameter)
      continue;

    /* Try slots in reverse (latest run first) */
    for (int i = (int)p.slots.size() - 1; i >= 0; --i) {
      const auto &slot = p.slots[i];
      unsigned run_h = (unsigned)std::atoi(slot.run_hour.c_str());

      /* How many hours ago did this run start? */
      int hours_since_run = (int)current_utc_hour - (int)run_h;
      if (hours_since_run < 0)
        hours_since_run += 24;

      /* The step we need */
      unsigned needed_step = (unsigned)(hours_since_run + (int)offset_hours);

      /* Check if this step is in range */
      if (needed_step >= slot.step_min && needed_step <= slot.step_max &&
          ((needed_step - slot.step_min) % slot.step_step == 0)) {
        out_date = slot.run_date;
        out_run_hour = slot.run_hour;
        out_step = needed_step;

        LogFmt("xctherm: FindSlotForOffset param={} now={}UTC +{}h -> "
               "run={}/{} step={} -> forecast {}UTC",
               parameter, current_utc_hour, offset_hours,
               slot.run_date, slot.run_hour, needed_step,
               (run_h + needed_step) % 24);
        return true;
      }
    }
  }

  LogFmt("xctherm: FindSlotForOffset: no slot for {} +{}h from {}UTC",
         parameter, offset_hours, current_utc_hour);
  return false;
}

/* ------------------------------------------------------------------ */
/* GeoJSON download                                                    */
/* ------------------------------------------------------------------ */

std::string
XCThermAPI::FormatStep(unsigned step) noexcept
{
  return std::string(FmtBuffer<8>("{:03}", step).c_str());
}

Co::Task<bool>
XCThermAPI::CoDownloadGeoJSON(CurlGlobal &curl,
                              const std::string &parameter,
                              const std::string &date,
                              const std::string &run_hour,
                              const unsigned step,
                              std::string &out_geojson,
                              int64_t *const out_wire_bytes,
                              ProgressListener *const progress,
                              const std::function<bool()> &should_continue)
{
  out_geojson.clear();

  if (!auth.EnsureValidToken()) {
    LogFmt("xctherm: cannot download — auth failed");
    throw XCThermAPIError(XCThermAPIError::Kind::AUTH_FAILED, 0,
                          "XCTherm authentication failed. Check "
                          "credentials in Config → System → Weather.");
  }

  const std::string step_str = FormatStep(step);
  const auto url = FmtBuffer<512>(
    "https://tiles.xctherm.com/forecast/{}/{}/{}/{}/{}.geojson",
    model, date, run_hour, step_str, parameter);
  const std::string context = parameter + " " + date + "/" + run_hour +
                              "Z step " + step_str;

  LogFmt("xctherm: downloading {}", url.c_str());

  Curl::CoResponse response;
  try {
    response = co_await XCTherm::Http::CoBearerGet(
      curl, url.c_str(), auth, progress, should_continue, true);
  } catch (const XCTherm::Http::TransferCancelled &) {
    co_return false;
  } catch (...) {
    if (!should_continue())
      co_return false;

    LogFmt("xctherm: download failed (network) {}", context);
    throw MakeApiError(true, 0, context);
  }

  if (!should_continue())
    co_return false;

  if (response.status != 200) {
    LogFmt("xctherm: download failed http={}", response.status);
    throw MakeApiError(response.status == 0, (long)response.status, context);
  }

  if (out_wire_bytes != nullptr)
    *out_wire_bytes = (int64_t)response.body.size();

  out_geojson = std::move(response.body);

  LogFmt("xctherm: {:.1f} KB decompressed",
         out_geojson.size() / 1024.0);

  /* Build the cache entry once; commit it atomically only after the
     curl handle reported success above. ControlsWidget reads from
     geojson_cache by UTC hour on the UI thread, so we must lock — but
     only across the short map insert, never across the curl call. */
  CachedSlice slice;
  slice.geojson = out_geojson;
  slice.run_date = date;
  slice.run_hour = run_hour;
  slice.step = step;
  slice.downloaded_at = (int64_t)std::chrono::duration_cast<std::chrono::seconds>(
    std::chrono::system_clock::now().time_since_epoch()).count();

  const unsigned run_h = (unsigned)std::atoi(run_hour.c_str());
  const unsigned forecast_utc = (run_h + step) % 24;

  /* Persist to disk before committing to RAM — that way a crash mid-
     write doesn't leave the RAM cache pointing at a slice that has no
     on-disk backing. Disk write is best-effort: failure logs, doesn't
     throw, doesn't prevent the RAM cache update below. */
  WriteSliceToDisk(parameter, forecast_utc, slice);

  {
    const std::lock_guard lock{cache_mutex};
    /* Record in the authoritative disk index... */
    disk_index[parameter][forecast_utc] =
      SliceMeta{slice.run_date, slice.run_hour, slice.step,
                slice.downloaded_at};
    /* ...and keep the just-downloaded body hot in the LRU cache. */
    InsertResident(parameter, forecast_utc, std::move(slice));
  }

  co_return !out_geojson.empty();
}

/* ------------------------------------------------------------------ */
/* Cache accessors                                                     */
/* ------------------------------------------------------------------ */

/* All cache accessors lock briefly to coordinate with the download
   worker thread that may be writing entries concurrently. */

bool
XCThermAPI::IsLayerCached(const std::string &parameter,
                           unsigned utc_hour) const noexcept
{
  const std::lock_guard lock{cache_mutex};
  auto it = disk_index.find(parameter);
  if (it == disk_index.end())
    return false;
  return it->second.find(utc_hour) != it->second.end();
}

bool
XCThermAPI::IsCachedAtRun(const std::string &parameter,
                          unsigned utc_hour,
                          const std::string &run_date,
                          const std::string &run_hour) const noexcept
{
  const std::lock_guard lock{cache_mutex};
  auto it = disk_index.find(parameter);
  if (it == disk_index.end())
    return false;
  auto it2 = it->second.find(utc_hour);
  if (it2 == it->second.end())
    return false;
  return it2->second.run_date == run_date
    && it2->second.run_hour == run_hour;
}

std::string
XCThermAPI::GetCachedGeoJSON(const std::string &parameter,
                             unsigned utc_hour) noexcept
{
  const std::lock_guard lock{cache_mutex};

  /* Fast path: body already resident. */
  auto it = geojson_cache.find(parameter);
  if (it != geojson_cache.end()) {
    auto it2 = it->second.find(utc_hour);
    if (it2 != it->second.end()) {
      TouchResident(parameter, utc_hour);
      return it2->second.geojson;
    }
  }

  /* Not resident — is it on disk (in the index)? */
  auto di = disk_index.find(parameter);
  if (di == disk_index.end() || di->second.find(utc_hour) == di->second.end())
    return {};

  /* Fault the body in from disk and keep it (LRU-bounded). The read
     happens under the lock; bodies are a few MB and reads are fast, and
     this is never called from the map draw thread. */
  CachedSlice slice;
  if (!ReadSliceFile(parameter, utc_hour, slice))
    return {};

  std::string body = slice.geojson;  // copy out before move into cache
  InsertResident(parameter, utc_hour, std::move(slice));
  return body;
}

std::optional<XCThermAPI::SliceMeta>
XCThermAPI::GetSliceMeta(const std::string &parameter,
                         unsigned utc_hour) const noexcept
{
  const std::lock_guard lock{cache_mutex};
  auto it = disk_index.find(parameter);
  if (it == disk_index.end())
    return std::nullopt;
  auto it2 = it->second.find(utc_hour);
  if (it2 == it->second.end())
    return std::nullopt;
  return it2->second;
}

std::vector<unsigned>
XCThermAPI::GetCachedHours(const std::string &parameter) const noexcept
{
  std::vector<unsigned> result;
  const std::lock_guard lock{cache_mutex};
  auto it = disk_index.find(parameter);
  if (it == disk_index.end())
    return result;
  for (const auto &kv : it->second)
    result.push_back(kv.first);
  std::sort(result.begin(), result.end());
  return result;
}

XCThermAPI::LayerCacheSummary
XCThermAPI::GetCachedLayerSummary(const std::string &parameter) const noexcept
{
  LayerCacheSummary out;
  const std::lock_guard lock{cache_mutex};
  auto it = disk_index.find(parameter);
  if (it == disk_index.end())
    return out;

  const auto now = std::chrono::system_clock::now();

  for (const auto &kv : it->second) {
    out.hours.push_back(kv.first);
    const auto &s = kv.second;
    if (s.downloaded_at != 0) {
      if (s.downloaded_at > out.latest_downloaded_at) {
        out.latest_downloaded_at = s.downloaded_at;
        /* Track the run identity of the most recently written slice
           — assuming all slices in one download share a run, this
           gives the dialog a coherent "Issued <run>" tag. */
        out.latest_run_date = s.run_date;
        out.latest_run_hour = s.run_hour;
      }
    }

    /* Count slices that are still valid in the future. valid_time =
       run_date (YYYYMMDD) + run_hour (HH) + step hours. Anything that
       fails to parse is silently skipped — we'd rather under-report
       than crash on malformed metadata. */
    if (s.run_date.size() == 8 && s.run_hour.size() == 2) {
      try {
        const int y = std::stoi(s.run_date.substr(0, 4));
        const unsigned mo = (unsigned)std::stoul(s.run_date.substr(4, 2));
        const unsigned d = (unsigned)std::stoul(s.run_date.substr(6, 2));
        const unsigned h = (unsigned)std::stoul(s.run_hour);
        const std::chrono::year_month_day ymd{
          std::chrono::year{y},
          std::chrono::month{mo},
          std::chrono::day{d}};
        if (ymd.ok()) {
          const auto valid_tp =
            std::chrono::sys_days{ymd}
            + std::chrono::hours{h}
            + std::chrono::hours{s.step};
          if (valid_tp > now)
            ++out.future_hours;
        }
      } catch (...) {
        /* skip */
      }
    }
  }
  std::sort(out.hours.begin(), out.hours.end());
  return out;
}

bool
XCThermAPI::HasAnyCache() const noexcept
{
  const std::lock_guard lock{cache_mutex};
  for (const auto &kv : disk_index)
    if (!kv.second.empty())
      return true;
  return false;
}

void
XCThermAPI::ClearLayer(const std::string &parameter) noexcept
{
  std::vector<unsigned> dropped_hours;
  {
    const std::lock_guard lock{cache_mutex};
    auto it = disk_index.find(parameter);
    if (it != disk_index.end()) {
      dropped_hours.reserve(it->second.size());
      for (const auto &kv : it->second)
        dropped_hours.push_back(kv.first);
    }
    disk_index.erase(parameter);
    geojson_cache.erase(parameter);
    for (unsigned utc : dropped_hours)
      DropResident(parameter, utc);
  }

  /* Disk cleanup outside the cache mutex — file I/O isn't a hot path
     and FileUtil functions are noexcept but can still take a moment. */
  for (unsigned utc : dropped_hours)
    DeleteSliceFromDisk(parameter, utc);
}

unsigned
XCThermAPI::PruneStaleRuns(const std::string &parameter,
                           unsigned current_utc_hour) noexcept
{
  /* Sweep policy: for every cached UTC hour of @p parameter, ask
     FindSlotForOffset which run is currently the freshest for that
     hour and drop the cache entry if it doesn't match.

     We compute the offset to the *next* occurrence of each hour from
     @c current_utc_hour. For past hours that means an offset in
     1..23 (wrapping into tomorrow); that's fine for matching the
     latest model run because runs don't depend on whether the
     forecast time has already passed.

     The lock is held briefly per lookup; FindSlotForOffset reads
     available_parameters which is only written by FetchIndex on the
     UI thread, so no second mutex needed. */
  std::vector<unsigned> hours_to_drop;

  {
    const std::lock_guard lock{cache_mutex};
    auto it = disk_index.find(parameter);
    if (it == disk_index.end())
      return 0;

    for (const auto &kv : it->second) {
      const unsigned utc = kv.first;
      const auto &meta = kv.second;

      const unsigned offset = (utc + 24 - current_utc_hour) % 24;
      std::string latest_date, latest_run;
      unsigned latest_step;
      if (!FindSlotForOffset(parameter, current_utc_hour, offset,
                             latest_date, latest_run, latest_step))
        /* Index doesn't cover this hour any more — keep the cached
           slice, the user explicitly downloaded it. */
        continue;

      if (meta.run_date != latest_date ||
          meta.run_hour != latest_run)
        hours_to_drop.push_back(utc);
    }

    for (unsigned utc : hours_to_drop) {
      it->second.erase(utc);
      DropResident(parameter, utc);
    }
  }

  for (unsigned utc : hours_to_drop)
    DeleteSliceFromDisk(parameter, utc);

  return (unsigned)hours_to_drop.size();
}

/* ------------------------------------------------------------------ */
/* Disk cache                                                          */
/* ------------------------------------------------------------------ */

/* On-disk file header: one-line metadata, then the original GeoJSON
   bytes. v=1 lets future versions invalidate older format silently. */
/* Current on-disk format marker. v2 added `downloaded_at=<unix_sec>`.
   v1 files are still loaded for backwards compatibility, with the
   timestamp falling back to the file mtime — see Loader::Visit. */
static constexpr const char *kDiskHeaderV1 = "#XCTHERMv1";
static constexpr const char *kDiskHeaderV2 = "#XCTHERMv2";
static constexpr const char *kDiskHeader = kDiskHeaderV2;

/* Files older than this on app startup are treated as stale and not
   loaded back into RAM (and deleted to keep the directory tidy). */
static constexpr std::chrono::hours kDiskTTL{24};

static AllocatedPath
BuildDiskCacheDirectory()
{
  const auto weather_path = LocalPath("weather");
  Directory::Create(weather_path);
  auto xctherm_path = AllocatedPath::Build(weather_path, Path("xctherm"));
  Directory::Create(xctherm_path);
  return xctherm_path;
}

void
XCThermAPI::EnableDiskCache() noexcept
{
  if (disk_cache_dir != nullptr)
    return;  /* idempotent */

  try {
    disk_cache_dir = BuildDiskCacheDirectory();
  } catch (...) {
    LogFmt("xctherm: could not create disk cache dir");
    disk_cache_dir = nullptr;
    return;
  }
  LogFmt("xctherm: disk cache dir = {}", disk_cache_dir.c_str());

  /* Scan existing files: index their headers (cheap), delete stale.
     Crucially we do NOT read the GeoJSON bodies here — only the small
     header line — so startup RAM stays low regardless of how many
     forecasts were downloaded. Bodies fault in on demand via
     GetCachedGeoJSON. Visitor captures `this`. */
  struct Indexer final : public File::Visitor {
    XCThermAPI &api;
    const std::chrono::system_clock::time_point cutoff;
    unsigned indexed = 0;
    unsigned dropped = 0;
    Indexer(XCThermAPI &_api,
            std::chrono::system_clock::time_point _cutoff) noexcept
      : api(_api), cutoff(_cutoff) {}

    void Visit(Path path, Path filename) override {
      /* Filename pattern: <param>_<utc>.xctcache */
      const auto mtime_tp = File::GetLastModification(path);
      if (mtime_tp < cutoff) {
        File::Delete(path);
        ++dropped;
        return;
      }

      /* Parse filename to recover (parameter, utc). */
      const std::string_view name{filename.c_str()};
      const auto dot = name.rfind('.');
      const auto under = name.rfind('_', dot);
      if (dot == std::string_view::npos ||
          under == std::string_view::npos ||
          under >= dot)
        return;
      const std::string parameter{name.substr(0, under)};
      unsigned utc;
      try {
        utc = (unsigned)std::stoul(std::string{name.substr(under + 1,
                                                           dot - under - 1)});
      } catch (...) {
        return;
      }
      if (utc >= 24)
        return;

      /* Read just the first chunk — enough to hold the one-line header
         (~80 bytes). We never pull the multi-MB body at startup. */
      std::string head;
      try {
        FileReader r{path};
        char buf[512];
        const auto n = r.Read(std::as_writable_bytes(std::span{buf}));
        head.assign(buf, n);
      } catch (...) {
        return;
      }

      const auto eol = head.find('\n');
      if (eol == std::string::npos)
        return;
      const bool is_v2 = head.compare(0, 10, kDiskHeaderV2) == 0;
      const bool is_v1 = head.compare(0, 10, kDiskHeaderV1) == 0;
      if (!is_v2 && !is_v1)
        return;

      /* v1: "#XCTHERMv1 run_date=YYYYMMDD run_hour=HH step=N"
         v2: same + " downloaded_at=<unix_sec>" */
      SliceMeta meta;
      auto extract = [&](const char *key) -> std::string_view {
        const std::string_view header(head.data(), eol);
        const std::string needle = std::string{key} + "=";
        const auto pos = header.find(needle);
        if (pos == std::string_view::npos)
          return {};
        const auto v_start = pos + needle.size();
        const auto v_end = header.find(' ', v_start);
        return header.substr(v_start,
          v_end == std::string_view::npos ? std::string_view::npos : v_end - v_start);
      };
      meta.run_date = std::string{extract("run_date")};
      meta.run_hour = std::string{extract("run_hour")};
      try {
        meta.step = (unsigned)std::stoul(std::string{extract("step")});
      } catch (...) {
        return;
      }
      if (meta.run_date.size() != 8 || meta.run_hour.size() != 2)
        return;

      /* v2: read explicit timestamp. v1: fall back to file mtime so old
         caches still display a sensible "downloaded at" in the dialog. */
      const auto dl_at = extract("downloaded_at");
      if (!dl_at.empty()) {
        try {
          meta.downloaded_at = (int64_t)std::stoll(std::string{dl_at});
        } catch (...) {
          meta.downloaded_at = 0;
        }
      }
      if (meta.downloaded_at == 0) {
        meta.downloaded_at =
          (int64_t)std::chrono::duration_cast<std::chrono::seconds>(
            mtime_tp.time_since_epoch()).count();
      }

      {
        const std::lock_guard lock{api.cache_mutex};
        api.disk_index[parameter][utc] = std::move(meta);
      }
      ++indexed;
    }
  };

  const auto now = std::chrono::system_clock::now();
  Indexer indexer(*this, now - kDiskTTL);
  Directory::VisitSpecificFiles(disk_cache_dir, "*.xctcache", indexer);
  LogFmt("xctherm: disk cache indexed {} slices (dropped {} stale)",
         indexer.indexed, indexer.dropped);
}

AllocatedPath
XCThermAPI::SliceFilePath(const std::string &parameter,
                          unsigned forecast_utc) const noexcept
{
  if (disk_cache_dir == nullptr)
    return nullptr;
  const auto name = FmtBuffer<128>("{}_{:02}.xctcache",
                                   parameter, forecast_utc);
  return AllocatedPath::Build(disk_cache_dir, name.c_str());
}

void
XCThermAPI::WriteSliceToDisk(const std::string &parameter,
                             unsigned forecast_utc,
                             const CachedSlice &slice) const noexcept
{
  const auto path = SliceFilePath(parameter, forecast_utc);
  if (path == nullptr)
    return;

  try {
    FileOutputStream out{path};
    const auto header = FmtBuffer<128>(
      "{} run_date={} run_hour={} step={} downloaded_at={}\n",
      kDiskHeader,
      slice.run_date, slice.run_hour, slice.step, slice.downloaded_at);
    out.Write(std::as_bytes(std::span(header.c_str(), std::strlen(header.c_str()))));
    out.Write(std::as_bytes(std::span(slice.geojson.data(),
                                      slice.geojson.size())));
    out.Commit();
  } catch (const std::exception &e) {
    LogFmt("xctherm: failed to persist {} @{:02}h: {}",
           parameter, forecast_utc, e.what());
  } catch (...) {
    /* never let disk failures escape — cache will simply be RAM-only
       this session. */
  }
}

void
XCThermAPI::DeleteSliceFromDisk(const std::string &parameter,
                                unsigned forecast_utc) const noexcept
{
  const auto path = SliceFilePath(parameter, forecast_utc);
  if (path == nullptr)
    return;
  File::Delete(path);
}

bool
XCThermAPI::ReadSliceFile(const std::string &parameter, unsigned utc_hour,
                          CachedSlice &out) const noexcept
{
  const auto path = SliceFilePath(parameter, utc_hour);
  if (path == nullptr)
    return false;

  std::string contents;
  try {
    FileReader r{path};
    char buf[64 * 1024];
    while (true) {
      const auto n = r.Read(std::as_writable_bytes(std::span{buf}));
      if (n == 0)
        break;
      contents.append(buf, n);
    }
  } catch (...) {
    return false;
  }

  const auto eol = contents.find('\n');
  if (eol == std::string::npos)
    return false;
  const bool is_v2 = contents.compare(0, 10, kDiskHeaderV2) == 0;
  const bool is_v1 = contents.compare(0, 10, kDiskHeaderV1) == 0;
  if (!is_v2 && !is_v1)
    return false;

  auto extract = [&](const char *key) -> std::string_view {
    const std::string_view header(contents.data(), eol);
    const std::string needle = std::string{key} + "=";
    const auto pos = header.find(needle);
    if (pos == std::string_view::npos)
      return {};
    const auto v_start = pos + needle.size();
    const auto v_end = header.find(' ', v_start);
    return header.substr(v_start,
      v_end == std::string_view::npos ? std::string_view::npos : v_end - v_start);
  };
  out.run_date = std::string{extract("run_date")};
  out.run_hour = std::string{extract("run_hour")};
  try {
    out.step = (unsigned)std::stoul(std::string{extract("step")});
  } catch (...) {
    return false;
  }
  const auto dl_at = extract("downloaded_at");
  if (!dl_at.empty()) {
    try {
      out.downloaded_at = (int64_t)std::stoll(std::string{dl_at});
    } catch (...) {
      out.downloaded_at = 0;
    }
  }
  out.geojson = contents.substr(eol + 1);
  return true;
}

void
XCThermAPI::TouchResident(const std::string &parameter,
                          unsigned utc_hour) noexcept
{
  const auto key = std::make_pair(parameter, utc_hour);
  auto it = std::find(lru_order.begin(), lru_order.end(), key);
  if (it != lru_order.end())
    lru_order.erase(it);
  lru_order.push_front(key);
}

void
XCThermAPI::DropResident(const std::string &parameter,
                         unsigned utc_hour) noexcept
{
  const auto key = std::make_pair(parameter, utc_hour);
  auto it = std::find(lru_order.begin(), lru_order.end(), key);
  if (it != lru_order.end())
    lru_order.erase(it);

  auto pit = geojson_cache.find(parameter);
  if (pit != geojson_cache.end()) {
    pit->second.erase(utc_hour);
    if (pit->second.empty())
      geojson_cache.erase(pit);
  }
}

void
XCThermAPI::InsertResident(const std::string &parameter, unsigned utc_hour,
                           CachedSlice &&slice) noexcept
{
  geojson_cache[parameter][utc_hour] = std::move(slice);
  TouchResident(parameter, utc_hour);

  /* Evict least-recently-used bodies beyond the budget. The disk index
     and the on-disk files are untouched — only the RAM body goes. */
  while (lru_order.size() > kMaxResidentSlices) {
    const auto victim = lru_order.back();
    lru_order.pop_back();
    auto pit = geojson_cache.find(victim.first);
    if (pit != geojson_cache.end()) {
      pit->second.erase(victim.second);
      if (pit->second.empty())
        geojson_cache.erase(pit);
    }
  }
}
