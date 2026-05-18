// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The XCSoar Project

#include "XCThermAPI.hpp"
#include "LogFile.hpp"

#include <algorithm>
#include <cstdio>
#include <cstring>
#include <ctime>
#include <set>
#include <vector>

#include <curl/curl.h>

const std::string XCThermAPI::kEmptyString;

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
 * Progress context passed to CURL xferinfo callback.
 * Updates the OperationEnvironment with MB downloaded and speed.
 */
struct CurlProgressCtx {
  OperationEnvironment *env;
  curl_off_t total_expected = 0;  // bytes expected (0 = unknown)
  char text_buf[128];
};

static int
CurlProgressCallback(void *userp,
                     curl_off_t dltotal, curl_off_t dlnow,
                     [[maybe_unused]] curl_off_t ultotal,
                     [[maybe_unused]] curl_off_t ulnow)
{
  auto *ctx = static_cast<CurlProgressCtx *>(userp);
  if (!ctx || !ctx->env)
    return 0;

  /* Update progress range when total becomes known */
  if (dltotal > 0 && ctx->total_expected != dltotal) {
    ctx->total_expected = dltotal;
    ctx->env->SetProgressRange(100);
  }

  /* Update progress position */
  if (dltotal > 0) {
    const int pct = (int)((dlnow * 100) / dltotal);
    ctx->env->SetProgressPosition(pct);
  }

  /* Build status text: MB downloaded + speed */
  double mb_now = (double)dlnow / (1024.0 * 1024.0);


  if (dltotal > 0) {
    double mb_total = (double)dltotal / (1024.0 * 1024.0);
    std::snprintf(ctx->text_buf, sizeof(ctx->text_buf),
                  "Downloading: %.2f / %.2f MB", mb_now, mb_total);
  } else {
    std::snprintf(ctx->text_buf, sizeof(ctx->text_buf),
                  "Downloading: %.2f MB", mb_now);
  }

  ctx->env->SetText(ctx->text_buf);
  return 0; /* non-zero = abort */
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
  }
}

/* ------------------------------------------------------------------ */
/* Index.json                                                          */
/* ------------------------------------------------------------------ */

bool
XCThermAPI::FetchIndex() noexcept
{
  LogFmt("xctherm: fetching index for model='{}'", model);

  const std::string url =
    "https://tiles.xctherm.com/forecast/" + model + "/index.json";

  CURL *curl = curl_easy_init();
  if (!curl)
    return false;

  std::vector<uint8_t> response_buffer;

  /* index.json is gzip-compressed; tell curl to decompress */
  curl_easy_setopt(curl, CURLOPT_URL, url.c_str());
  curl_easy_setopt(curl, CURLOPT_ACCEPT_ENCODING, "gzip");
  curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, CurlWriteCallback);
  curl_easy_setopt(curl, CURLOPT_WRITEDATA, &response_buffer);
  curl_easy_setopt(curl, CURLOPT_CONNECTTIMEOUT, 10L);
  curl_easy_setopt(curl, CURLOPT_TIMEOUT, 20L);
  curl_easy_setopt(curl, CURLOPT_SSL_VERIFYPEER, 0L);
  curl_easy_setopt(curl, CURLOPT_SSL_VERIFYHOST, 0L);
  curl_easy_setopt(curl, CURLOPT_NOSIGNAL, 1L);

  CURLcode res = curl_easy_perform(curl);
  long http_code = 0;
  curl_easy_getinfo(curl, CURLINFO_RESPONSE_CODE, &http_code);
  curl_easy_cleanup(curl);

  if (res != CURLE_OK || http_code != 200) {
    LogFmt("xctherm: index fetch failed curl={} http={}", (int)res, http_code);
    return false;
  }

  std::string json(response_buffer.begin(), response_buffer.end());
  LogFmt("xctherm: index fetched, {} bytes", json.size());

  return ParseIndex(json);
}

/* ------------------------------------------------------------------ */
/* Simple JSON parsing (no external library)                           */
/* ------------------------------------------------------------------ */

/**
 * Find a JSON string value: "key": "value" (handles optional whitespace)
 * Returns empty string if not found.
 */
static std::string
JsonFindString(const std::string &json, size_t start,
               const char *key) noexcept
{
  std::string needle = std::string("\"") + key + "\"";
  size_t pos = json.find(needle, start);
  if (pos == std::string::npos)
    return {};

  pos += needle.size();
  /* Skip : and whitespace */
  while (pos < json.size() && (json[pos] == ':' || json[pos] == ' '))
    ++pos;
  /* Expect opening quote */
  if (pos >= json.size() || json[pos] != '"')
    return {};
  ++pos;

  size_t end = json.find('"', pos);
  if (end == std::string::npos)
    return {};

  return json.substr(pos, end - pos);
}

/**
 * Find a JSON number value: "key": <number> (handles optional whitespace)
 */
static int
JsonFindInt(const std::string &json, size_t start,
            const char *key, int default_val = 0) noexcept
{
  std::string needle = std::string("\"") + key + "\"";
  size_t pos = json.find(needle, start);
  if (pos == std::string::npos)
    return default_val;

  pos += needle.size();
  /* Skip : and whitespace */
  while (pos < json.size() && (json[pos] == ':' || json[pos] == ' '))
    ++pos;

  return std::atoi(json.c_str() + pos);
}

bool
XCThermAPI::ParseIndex(const std::string &json) noexcept
{
  available_parameters.clear();
  index_loaded = false;

  /* Find "parameters" object */
  size_t params_pos = json.find("\"parameters\"");
  if (params_pos == std::string::npos) {
    LogFmt("xctherm: no 'parameters' in index.json");
    return false;
  }

  /* Scan for vertical_wind parameters */
  const char *vw_prefix = "\"vertical_wind_";
  size_t search_pos = params_pos;

  while (true) {
    size_t pos = json.find(vw_prefix, search_pos);
    if (pos == std::string::npos)
      break;

    /* Extract parameter name */
    pos += 1; // skip opening quote
    size_t name_end = json.find('"', pos);
    if (name_end == std::string::npos)
      break;

    std::string param_name = json.substr(pos, name_end - pos);
    search_pos = name_end + 1;

    /* Find the slots array for this parameter */
    size_t slots_pos = json.find("\"slots\"", search_pos);
    if (slots_pos == std::string::npos)
      break;

    /* Find the opening bracket of the slots array */
    size_t arr_start = json.find('[', slots_pos);
    if (arr_start == std::string::npos)
      break;

    /* Find matching closing bracket (handle nesting) */
    int depth = 1;
    size_t arr_end = arr_start + 1;
    while (arr_end < json.size() && depth > 0) {
      if (json[arr_end] == '[') ++depth;
      else if (json[arr_end] == ']') --depth;
      ++arr_end;
    }

    ParameterInfo info;
    info.name = param_name;

    /* Parse each slot object within the array */
    size_t slot_search = arr_start;
    while (true) {
      size_t slot_start = json.find('{', slot_search);
      if (slot_start == std::string::npos || slot_start >= arr_end)
        break;

      /* Find matching closing brace (handle nested "steps": {...}) */
      int obj_depth = 1;
      size_t slot_end = slot_start + 1;
      while (slot_end < json.size() && obj_depth > 0) {
        if (json[slot_end] == '{') ++obj_depth;
        else if (json[slot_end] == '}') --obj_depth;
        ++slot_end;
      }
      --slot_end; /* point at the closing brace */
      if (obj_depth != 0)
        break;

      /* Parse "run" datetime */
      std::string run_str = JsonFindString(json, slot_start, "run");
      if (run_str.empty()) {
        slot_search = slot_end + 1;
        continue;
      }

      ForecastSlot slot;

      /* Parse run datetime: "2026-05-06T03:00:00+00:00" */
      /* Extract date YYYYMMDD */
      if (run_str.size() >= 10) {
        slot.run_date = run_str.substr(0, 4) +
                        run_str.substr(5, 2) +
                        run_str.substr(8, 2);
      }
      /* Extract hour HH */
      if (run_str.size() >= 13) {
        slot.run_hour = run_str.substr(11, 2);
      }

      /* Parse steps */
      size_t steps_pos = json.find("\"steps\"", slot_start);
      if (steps_pos != std::string::npos && steps_pos <= slot_end) {
        slot.step_min = (unsigned)JsonFindInt(json, steps_pos, "min", 0);
        slot.step_max = (unsigned)JsonFindInt(json, steps_pos, "max", 0);
        slot.step_step = (unsigned)JsonFindInt(json, steps_pos, "step", 1);
        if (slot.step_step == 0) slot.step_step = 1;
      }

      info.slots.push_back(std::move(slot));
      slot_search = slot_end + 1;
    }

    if (!info.slots.empty()) {
      LogFmt("xctherm: param '{}' — {} slots", info.name, info.slots.size());
      available_parameters.push_back(std::move(info));
    }

    search_pos = arr_end;
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
/* Find best slot for a target hour                                    */
/* ------------------------------------------------------------------ */

bool
XCThermAPI::FindSlotForHour(const std::string &parameter,
                            unsigned target_utc_hour,
                            std::string &out_date,
                            std::string &out_run_hour,
                            unsigned &out_step) const noexcept
{
  for (const auto &p : available_parameters) {
    if (p.name != parameter)
      continue;

    /* Prefer the latest run (last slot in the list) */
    for (int i = (int)p.slots.size() - 1; i >= 0; --i) {
      const auto &slot = p.slots[i];
      unsigned run_h = (unsigned)std::atoi(slot.run_hour.c_str());

      for (unsigned s = slot.step_min; s <= slot.step_max; s += slot.step_step) {
        if ((run_h + s) % 24 == target_utc_hour) {
          out_date = slot.run_date;
          out_run_hour = slot.run_hour;
          out_step = s;
          return true;
        }
      }
    }
  }

  return false;
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

bool
XCThermAPI::DownloadGeoJSON(const std::string &parameter,
                            const std::string &date,
                            const std::string &run_hour,
                            unsigned step,
                            std::string &out_geojson,
                            OperationEnvironment *env,
                            int64_t *out_wire_bytes) noexcept
{
  out_geojson.clear();

  /* Ensure we have a valid auth token */
  if (env) env->SetText("Authenticating...");

  if (!auth.EnsureValidToken()) {
    LogFmt("xctherm: cannot download — auth failed");
    return false;
  }

  const std::string step_str = FormatStep(step);
  const std::string url =
    "https://tiles.xctherm.com/forecast/" + model + "/" +
    date + "/" + run_hour + "/" + step_str + "/" + parameter + ".geojson";

  LogFmt("xctherm: downloading {}", url);

  if (env) {
    char msg[128];
    std::snprintf(msg, sizeof(msg), "Connecting to server...");
    env->SetText(msg);
    env->SetProgressRange(0);
  }

  CURL *curl = curl_easy_init();
  if (!curl)
    return false;

  std::vector<uint8_t> response_buffer;
  CurlProgressCtx progress_ctx;
  progress_ctx.env = env;
  progress_ctx.total_expected = 0;

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
  curl_easy_setopt(curl, CURLOPT_SSL_VERIFYPEER, 0L);
  curl_easy_setopt(curl, CURLOPT_SSL_VERIFYHOST, 0L);
  curl_easy_setopt(curl, CURLOPT_NOSIGNAL, 1L);
  curl_easy_setopt(curl, CURLOPT_FOLLOWLOCATION, 1L);

  if (env) {
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
    LogFmt("xctherm: 401, re-authenticating");
    if (!auth.ForceReauthenticate())
      return false;
    return DownloadGeoJSON(parameter, date, run_hour, step, out_geojson, env);
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

  /* Store in cache so ControlsWidget can use without re-downloading */
  geojson_cache[parameter][step] = out_geojson;
  /* Also index by the actual UTC hour this step corresponds to */
  {
    unsigned run_h = (unsigned)std::atoi(run_hour.c_str());
    unsigned forecast_utc = (run_h + step) % 24;
    geojson_cache[parameter][forecast_utc] = out_geojson;
  }

  return !out_geojson.empty();
}

bool
XCThermAPI::DownloadLayerForHour(const std::string &parameter,
                                 unsigned target_utc_hour,
                                 std::string &out_geojson,
                                 OperationEnvironment *env) noexcept
{
  std::string date, run_hour;
  unsigned step;

  if (!FindSlotForHour(parameter, target_utc_hour, date, run_hour, step)) {
    LogFmt("xctherm: no slot for {}@{}h", parameter, target_utc_hour);
    return false;
  }

  return DownloadGeoJSON(parameter, date, run_hour, step, out_geojson, env);
}

/* ------------------------------------------------------------------ */
/* Cache accessors                                                     */
/* ------------------------------------------------------------------ */

bool
XCThermAPI::IsLayerCached(const std::string &parameter,
                           unsigned utc_hour) const noexcept
{
  auto it = geojson_cache.find(parameter);
  if (it == geojson_cache.end())
    return false;
  return it->second.find(utc_hour) != it->second.end();
}

const std::string &
XCThermAPI::GetCachedGeoJSON(const std::string &parameter,
                              unsigned utc_hour) const noexcept
{
  auto it = geojson_cache.find(parameter);
  if (it == geojson_cache.end())
    return kEmptyString;
  auto it2 = it->second.find(utc_hour);
  if (it2 == it->second.end())
    return kEmptyString;
  return it2->second;
}

std::vector<unsigned>
XCThermAPI::GetCachedHours(const std::string &parameter) const noexcept
{
  std::vector<unsigned> result;
  auto it = geojson_cache.find(parameter);
  if (it == geojson_cache.end())
    return result;
  for (const auto &kv : it->second)
    result.push_back(kv.first);
  std::sort(result.begin(), result.end());
  return result;
}

std::vector<std::string>
XCThermAPI::GetCachedParameters() const noexcept
{
  std::vector<std::string> result;
  for (const auto &kv : geojson_cache)
    if (!kv.second.empty())
      result.push_back(kv.first);
  return result;
}

void
XCThermAPI::ClearCache() noexcept
{
  geojson_cache.clear();
}
