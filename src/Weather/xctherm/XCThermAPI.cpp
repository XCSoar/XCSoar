// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The XCSoar Project

#include "XCThermAPI.hpp"
#include "LogFile.hpp"
#include "LocalPath.hpp"
#include "Weather/xctherm/XCThermGeoJSON.hpp"
#include "io/FileOutputStream.hxx"
#include "system/FileUtil.hpp"
#include "util/StaticString.hxx"

#include <algorithm>
#include <cstdio>
#include <cstring>
#include <ctime>
#include <exception>
#include <set>
#include <span>
#include <vector>

#include <boost/json.hpp>
#include <curl/curl.h>

/* ------------------------------------------------------------------ */
/* CURL helpers                                                        */
/* ------------------------------------------------------------------ */

static size_t
CurlWriteCallback(void *contents, size_t size, size_t nmemb,
                  std::vector<uint8_t> *buffer) {
  const size_t total = size * nmemb;
  auto *ptr = static_cast<uint8_t *>(contents);
  buffer->insert(buffer->end(), ptr, ptr + total);
  return total;
}

/**
 * Progress context passed to CURL xferinfo callback. The callback
 * forwards (bytes_now, bytes_total) to the caller-supplied ProgressFn
 * and aborts the transfer when it returns false (Stop pressed).
 */
struct CurlProgressCtx {
  const XCThermAPI::ProgressFn *progress = nullptr;
};

static int
CurlProgressCallback(void *userp,
                     curl_off_t dltotal, curl_off_t dlnow,
                     [[maybe_unused]] curl_off_t ultotal,
                     [[maybe_unused]] curl_off_t ulnow)
{
  auto *ctx = static_cast<CurlProgressCtx *>(userp);
  if (ctx == nullptr || ctx->progress == nullptr || !*ctx->progress)
    return 0;

  /* Returning non-zero aborts curl with CURLE_ABORTED_BY_CALLBACK,
     which is how the Stop button becomes effective mid-transfer. */
  return (*ctx->progress)((uint64_t)dlnow, (uint64_t)dltotal) ? 0 : 1;
}

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
    {
      const std::lock_guard lock{cache_mutex};
      geojson_cache.clear();
    }
    disk_cache_loaded = false;
  }
}

/* ------------------------------------------------------------------ */
/* Index.json                                                          */
/* ------------------------------------------------------------------ */

static bool WriteTextFile(Path path, std::string_view content) noexcept;

bool
XCThermAPI::FetchIndex() noexcept
{
  LogFmt("xctherm: fetching index for model='{}'", model);

  const std::string url =
    "https://tiles.xctherm.com/forecast/" + model + "/index.json";

  CURL *curl = curl_easy_init();
  if (!curl) {
    const std::string cached = XCThermGeoJSON::ReadFile(BuildIndexPath());
    if (!cached.empty())
      return ParseIndex(cached);
    return false;
  }

  std::vector<uint8_t> response_buffer;

  /* index.json is gzip-compressed; tell curl to decompress */
  curl_easy_setopt(curl, CURLOPT_URL, url.c_str());
  curl_easy_setopt(curl, CURLOPT_ACCEPT_ENCODING, "gzip");
  curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, CurlWriteCallback);
  curl_easy_setopt(curl, CURLOPT_WRITEDATA, &response_buffer);
  curl_easy_setopt(curl, CURLOPT_CONNECTTIMEOUT, 10L);
  curl_easy_setopt(curl, CURLOPT_TIMEOUT, 20L);
  curl_easy_setopt(curl, CURLOPT_NOSIGNAL, 1L);

  CURLcode res = curl_easy_perform(curl);
  long http_code = 0;
  curl_easy_getinfo(curl, CURLINFO_RESPONSE_CODE, &http_code);
  curl_easy_cleanup(curl);

  if (res != CURLE_OK || http_code != 200) {
    LogFmt("xctherm: index fetch failed curl={} http={}", (int)res, http_code);
    const std::string cached = XCThermGeoJSON::ReadFile(BuildIndexPath());
    if (!cached.empty()) {
      LogFmt("xctherm: using cached index.json ({} bytes)", cached.size());
      return ParseIndex(cached);
    }
    return false;
  }

  std::string json(response_buffer.begin(), response_buffer.end());
  LogFmt("xctherm: index fetched, {} bytes", json.size());

  WriteTextFile(BuildIndexPath(), json);
  return ParseIndex(json);
}

/* ------------------------------------------------------------------ */
/* Index.json parsing                                                  */
/* ------------------------------------------------------------------ */

static unsigned
GetUintMember(const boost::json::object &obj, const char *key,
              unsigned default_val) noexcept
{
  const auto *v = obj.if_contains(key);
  if (v == nullptr)
    return default_val;

  if (v->is_uint64())
    return (unsigned)v->as_uint64();

  if (v->is_int64()) {
    const auto n = v->as_int64();
    return n >= 0 ? (unsigned)n : default_val;
  }

  return default_val;
}

bool
XCThermAPI::ParseIndex(const std::string &json) noexcept
{
  available_parameters.clear();
  index_loaded = false;

  try {
    const boost::json::value jv = boost::json::parse(json);
    if (!jv.is_object())
      return false;

    const auto *parameters = jv.as_object().if_contains("parameters");
    if (parameters == nullptr || !parameters->is_object()) {
      LogFmt("xctherm: no 'parameters' in index.json");
      return false;
    }

    for (const auto &[name, param_v] : parameters->as_object()) {
      if (!name.starts_with("vertical_wind_") || !param_v.is_object())
        continue;

      const auto *slots = param_v.as_object().if_contains("slots");
      if (slots == nullptr || !slots->is_array())
        continue;

      ParameterInfo info;
      info.name = std::string{name};

      for (const auto &slot_v : slots->as_array()) {
        if (!slot_v.is_object())
          continue;

        const auto &slot_obj = slot_v.as_object();
        const auto *run_v = slot_obj.if_contains("run");
        if (run_v == nullptr || !run_v->is_string())
          continue;

        const std::string run_str = std::string{run_v->as_string()};
        if (run_str.size() < 13)
          continue;

        ForecastSlot slot;
        slot.run_date = run_str.substr(0, 4) +
                        run_str.substr(5, 2) +
                        run_str.substr(8, 2);
        slot.run_hour = run_str.substr(11, 2);

        const auto *steps_v = slot_obj.if_contains("steps");
        if (steps_v != nullptr && steps_v->is_object()) {
          const auto &steps = steps_v->as_object();
          slot.step_min = GetUintMember(steps, "min", 0);
          slot.step_max = GetUintMember(steps, "max", 0);
          slot.step_step = GetUintMember(steps, "step", 1);
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
  } catch (const std::exception &e) {
    LogFmt("xctherm: index parse failed: {}", e.what());
    return false;
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
  char buf[4];
  std::snprintf(buf, sizeof(buf), "%03u", step);
  return buf;
}

/* ------------------------------------------------------------------ */
/* On-disk cache (GetCachePath + LocalPath/weather/xctherm)            */
/* ------------------------------------------------------------------ */

static bool
WriteTextFile(Path path, std::string_view content) noexcept
{
  try {
    FileOutputStream file(path);
    file.Write(std::span{
      reinterpret_cast<const std::byte *>(content.data()), content.size()});
    file.Commit();
    return true;
  } catch (...) {
    LogFmt("xctherm: failed to write file {}", path.c_str());
    return false;
  }
}

AllocatedPath
XCThermAPI::BuildIndexPath() const noexcept
{
  const auto cache_root = MakeCacheDirectory("xctherm");
  const auto model_path = AllocatedPath::Build(cache_root, model.c_str());
  Directory::Create(model_path);
  return AllocatedPath::Build(model_path, "index.json");
}

bool
XCThermAPI::ParseSliceBasename(std::string_view base,
                               std::string &parameter,
                               std::string &run_date,
                               std::string &run_hour,
                               unsigned &step) noexcept
{
  constexpr std::string_view suffix = ".geojson";
  if (base.size() <= suffix.size() ||
      base.substr(base.size() - suffix.size()) != suffix)
    return false;
  base.remove_suffix(suffix.size());

  const auto strip_segment = [&base](std::size_t len) -> std::string {
    if (base.size() < len + 1 || base[base.size() - len - 1] != '_')
      return {};
    const std::string segment(base.substr(base.size() - len));
    base.remove_suffix(len + 1);
    return segment;
  };

  const std::string step_str = strip_segment(3);
  if (step_str.empty())
    return false;
  step = (unsigned)std::atoi(step_str.c_str());

  run_hour = strip_segment(2);
  if (run_hour.size() != 2)
    return false;

  run_date = strip_segment(8);
  if (run_date.size() != 8)
    return false;

  if (base.empty())
    return false;

  parameter = std::string(base);
  return true;
}

AllocatedPath
XCThermAPI::BuildSlicePath(const std::string &parameter,
                           const std::string &run_date,
                           const std::string &run_hour,
                           unsigned step) const noexcept
{
  StaticString<256> basename;
  basename.Format("%s_%s_%s_%03u.geojson",
                  parameter.c_str(), run_date.c_str(),
                  run_hour.c_str(), step);

  const auto cache_root = MakeCacheDirectory("xctherm");
  const auto model_path = AllocatedPath::Build(cache_root, model.c_str());
  Directory::Create(model_path);
  return AllocatedPath::Build(model_path, basename);
}

bool
XCThermAPI::SaveSliceToDisk(const CachedSlice &slice,
                            const std::string &parameter) const noexcept
{
  if (slice.geojson.empty())
    return false;

  const auto path = BuildSlicePath(parameter, slice.run_date,
                                   slice.run_hour, slice.step);
  return WriteTextFile(path, slice.geojson);
}

void
XCThermAPI::ImportCacheFile(Path path, Path filename) noexcept
{
  std::string parameter, run_date, run_hour;
  unsigned step = 0;
  if (!ParseSliceBasename(filename.GetBase().c_str(),
                          parameter, run_date, run_hour, step))
    return;

  const std::string geojson = XCThermGeoJSON::ReadFile(path);
  if (geojson.empty())
    return;

  CachedSlice slice;
  slice.geojson = geojson;
  slice.run_date = std::move(run_date);
  slice.run_hour = std::move(run_hour);
  slice.step = step;

  const unsigned run_h = (unsigned)std::atoi(slice.run_hour.c_str());
  const unsigned forecast_utc = (run_h + step) % 24;

  const std::lock_guard lock{cache_mutex};
  geojson_cache[parameter][forecast_utc] = std::move(slice);
}

void
XCThermAPI::LoadCacheFromDisk() noexcept
{
  struct Visitor final : public File::Visitor {
    XCThermAPI &api;

    explicit Visitor(XCThermAPI &_api):api(_api) {}

    void Visit(Path path, Path filename) override {
      api.ImportCacheFile(path, filename);
    }
  };

  const auto cache_root = MakeCacheDirectory("xctherm");
  const auto model_cache = AllocatedPath::Build(cache_root, model.c_str());
  Directory::Create(model_cache);
  Visitor visitor{*this};
  Directory::VisitSpecificFiles(model_cache, "*.geojson", visitor);

  const auto weather_root =
    AllocatedPath::Build(MakeLocalPath("weather"), "xctherm");
  Directory::Create(weather_root);
  const auto model_weather =
    AllocatedPath::Build(weather_root, model.c_str());
  Directory::Create(model_weather);
  Directory::VisitSpecificFiles(model_weather, "*.geojson", visitor, true);

  disk_cache_loaded = true;
  LogFmt("xctherm: loaded disk cache for model '{}'", model);
}

void
XCThermAPI::LoadDiskCache() noexcept
{
  if (disk_cache_loaded)
    return;
  LoadCacheFromDisk();
}

bool
XCThermAPI::DownloadGeoJSON(const std::string &parameter,
                            const std::string &date,
                            const std::string &run_hour,
                            unsigned step,
                            std::string &out_geojson,
                            int64_t *out_wire_bytes,
                            ProgressFn progress) noexcept
{
  return DownloadGeoJSONOnce(parameter, date, run_hour, step, out_geojson,
                             out_wire_bytes, std::move(progress), false);
}

bool
XCThermAPI::DownloadGeoJSONOnce(const std::string &parameter,
                                const std::string &date,
                                const std::string &run_hour,
                                unsigned step,
                                std::string &out_geojson,
                                int64_t *out_wire_bytes,
                                ProgressFn progress,
                                bool reauth_attempted) noexcept
{
  out_geojson.clear();

  if (!auth.EnsureValidToken()) {
    LogFmt("xctherm: cannot download — auth failed");
    return false;
  }

  const std::string step_str = FormatStep(step);
  const std::string url =
    "https://tiles.xctherm.com/forecast/" + model + "/" +
    date + "/" + run_hour + "/" + step_str + "/" + parameter + ".geojson";

  LogFmt("xctherm: downloading {}", url);

  CURL *curl = curl_easy_init();
  if (!curl)
    return false;

  std::vector<uint8_t> response_buffer;
  CurlProgressCtx progress_ctx;
  progress_ctx.progress = progress ? &progress : nullptr;

  struct curl_slist *headers = nullptr;
  const std::string auth_header = auth.GetAuthHeader();
  headers = curl_slist_append(headers, auth_header.c_str());

  curl_easy_setopt(curl, CURLOPT_URL, url.c_str());
  curl_easy_setopt(curl, CURLOPT_HTTPHEADER, headers);
  curl_easy_setopt(curl, CURLOPT_ACCEPT_ENCODING, "gzip");
  curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, CurlWriteCallback);
  curl_easy_setopt(curl, CURLOPT_WRITEDATA, &response_buffer);
  curl_easy_setopt(curl, CURLOPT_CONNECTTIMEOUT, 10L);
  curl_easy_setopt(curl, CURLOPT_TIMEOUT, 60L);
  curl_easy_setopt(curl, CURLOPT_NOSIGNAL, 1L);
  curl_easy_setopt(curl, CURLOPT_FOLLOWLOCATION, 1L);

  if (progress) {
    /* Wire the progress callback so the worker thread can update the
       per-slice byte counter and, by returning false, abort mid-fetch
       when the user presses Stop. */
    curl_easy_setopt(curl, CURLOPT_XFERINFOFUNCTION, CurlProgressCallback);
    curl_easy_setopt(curl, CURLOPT_XFERINFODATA, &progress_ctx);
    curl_easy_setopt(curl, CURLOPT_NOPROGRESS, 0L);
  }

  CURLcode res = curl_easy_perform(curl);
  long http_code = 0;
  curl_easy_getinfo(curl, CURLINFO_RESPONSE_CODE, &http_code);

  /* Wire size = bytes actually received over the network (compressed) */
  curl_off_t wire_bytes = 0;
  curl_easy_getinfo(curl, CURLINFO_SIZE_DOWNLOAD_T, &wire_bytes);

  /* Average download speed (bytes/sec) */
  curl_off_t speed_bps = 0;
  curl_easy_getinfo(curl, CURLINFO_SPEED_DOWNLOAD_T, &speed_bps);

  curl_slist_free_all(headers);
  curl_easy_cleanup(curl);

  if (out_wire_bytes)
    *out_wire_bytes = (int64_t)wire_bytes;

  /* Handle 401: try re-auth once */
  if (http_code == 401) {
    if (reauth_attempted) {
      LogFmt("xctherm: 401 after re-auth");
      return false;
    }
    LogFmt("xctherm: 401, re-authenticating");
    if (!auth.ForceReauthenticate())
      return false;
    return DownloadGeoJSONOnce(parameter, date, run_hour, step, out_geojson,
                               out_wire_bytes, std::move(progress), true);
  }

  if (res != CURLE_OK || http_code != 200) {
    LogFmt("xctherm: download failed curl={} http={}", (int)res, http_code);
    return false;
  }

  out_geojson.assign(response_buffer.begin(), response_buffer.end());

  LogFmt("xctherm: {:.1f} KB over wire -> {:.1f} KB decompressed ({:.1f}x), {:.0f} KB/s",
         wire_bytes / 1024.0,
         out_geojson.size() / 1024.0,
         wire_bytes > 0 ? (double)out_geojson.size() / wire_bytes : 1.0,
         speed_bps / 1024.0);

  /* Build the cache entry once; commit it atomically only after the
     curl handle reported success above. ControlsWidget reads from
     geojson_cache by UTC hour on the UI thread, so we must lock — but
     only across the short map insert, never across the curl call. */
  CachedSlice slice;
  slice.geojson = out_geojson;
  slice.run_date = date;
  slice.run_hour = run_hour;
  slice.step = step;

  const unsigned run_h = (unsigned)std::atoi(run_hour.c_str());
  const unsigned forecast_utc = (run_h + step) % 24;
  {
    const std::lock_guard lock{cache_mutex};
    geojson_cache[parameter][forecast_utc] = slice;
  }
  SaveSliceToDisk(slice, parameter);

  return !out_geojson.empty();
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
  auto it = geojson_cache.find(parameter);
  if (it == geojson_cache.end())
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
  auto it = geojson_cache.find(parameter);
  if (it == geojson_cache.end())
    return false;
  auto it2 = it->second.find(utc_hour);
  if (it2 == it->second.end())
    return false;
  return it2->second.run_date == run_date
    && it2->second.run_hour == run_hour;
}

std::string
XCThermAPI::GetCachedGeoJSON(const std::string &parameter,
                             unsigned utc_hour) const noexcept
{
  const std::lock_guard lock{cache_mutex};
  auto it = geojson_cache.find(parameter);
  if (it == geojson_cache.end())
    return {};
  auto it2 = it->second.find(utc_hour);
  if (it2 == it->second.end())
    return {};
  return it2->second.geojson;
}

bool
XCThermAPI::GetCachedSlice(const std::string &parameter,
                           unsigned utc_hour,
                           CachedSlice &out) const noexcept
{
  const std::lock_guard lock{cache_mutex};
  auto it = geojson_cache.find(parameter);
  if (it == geojson_cache.end())
    return false;
  auto it2 = it->second.find(utc_hour);
  if (it2 == it->second.end())
    return false;
  out = it2->second;
  return true;
}

std::vector<unsigned>
XCThermAPI::GetCachedHours(const std::string &parameter) const noexcept
{
  std::vector<unsigned> result;
  const std::lock_guard lock{cache_mutex};
  auto it = geojson_cache.find(parameter);
  if (it == geojson_cache.end())
    return result;
  for (const auto &kv : it->second)
    result.push_back(kv.first);
  std::sort(result.begin(), result.end());
  return result;
}

void
XCThermAPI::ClearCache() noexcept
{
  {
    const std::lock_guard lock{cache_mutex};
    geojson_cache.clear();
  }

  struct Deleter final : public File::Visitor {
    void Visit(Path path, Path) override {
      File::Delete(path);
    }
  } deleter;

  const auto cache_root = MakeCacheDirectory("xctherm");
  const auto model_path = AllocatedPath::Build(cache_root, model.c_str());
  if (Directory::Exists(model_path))
    Directory::VisitSpecificFiles(model_path, "*.geojson", deleter);
}
