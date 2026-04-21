// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The XCSoar Project

#include "XCThermAPI.hpp"
#include "XCThermOverlay.hpp"
#include "MVT.hpp"
#include "TileCoord.hpp"
#include "LogFile.hpp"
#include "Operation/Operation.hpp"

#include <algorithm>
#include <cmath>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <ctime>
#include <fstream>
#include <string>
#include <thread>
#include <chrono>
#include <vector>
#include <sys/stat.h>

#include <curl/curl.h>

/* ------------------------------------------------------------------ */
/* Helpers                                                             */
/* ------------------------------------------------------------------ */

static size_t
CurlWriteCallback(void *contents, size_t size, size_t nmemb,
                  std::vector<uint8_t> *buffer) {
  const size_t total = size * nmemb;
  auto *ptr = static_cast<uint8_t *>(contents);
  buffer->insert(buffer->end(), ptr, ptr + total);
  return total;
}

static bool
EnsureDirectoryRecursive(const std::string &path) {
  if (path.empty())
    return false;

  std::string current;
  current.reserve(path.size());

  for (char c : path) {
    current.push_back(c);
    if (c == '/' && current.size() > 1)
      (void)::mkdir(current.c_str(), 0755);
  }

  (void)::mkdir(current.c_str(), 0755);
  return true;
}

static void
AddCurlResolve(curl_slist *&list, const char *host, const char *ip) {
  const std::string entry = std::string(host) + ":443:" + ip;
  list = curl_slist_append(list, entry.c_str());
}

static const char *
DefaultCacheBasePath() {
#ifdef __APPLE__
  return "/tmp/xcsoar_xctherm";
#elif defined(__ANDROID__)
  return "/sdcard/Android/data/org.xcsoar/cache/xctherm";
#else
  return "/tmp/xcsoar_xctherm";
#endif
}

static int
CurrentUtcHour() {
  std::time_t now = std::time(nullptr);
  std::tm *utc = std::gmtime(&now);
  if (utc == nullptr)
    return 0;
  return std::clamp(utc->tm_hour, 0, 23);
}

static std::vector<const char *>
BuildPreferredSlots(const std::string &run_cycle) {
  static constexpr const char *kSlots[] = {"000", "001", "002"};

  int run_hour = std::clamp(std::atoi(run_cycle.c_str()), 0, 23);
  const int utc_hour = CurrentUtcHour();
  const int lead_hours = (utc_hour - run_hour + 24) % 24;

  int preferred = 2;
  if (lead_hours >= 0 && lead_hours <= 2)
    preferred = lead_hours;

  std::vector<const char *> ordered;
  ordered.reserve(3);
  ordered.push_back(kSlots[preferred]);
  for (int i = 0; i < 3; ++i)
    if (i != preferred)
      ordered.push_back(kSlots[i]);

  return ordered;
}

/* ------------------------------------------------------------------ */
/* XCThermAPI Singleton                                                */
/* ------------------------------------------------------------------ */

XCThermAPI &
XCThermAPI::Instance() {
  static XCThermAPI instance;
  return instance;
}

XCThermAPI::XCThermAPI() {
  settings.SetDefaults();
}

XCThermAPI::~XCThermAPI() = default;

void
XCThermAPI::UpdateSettings(const Settings &new_settings) {
  const std::string prev_email = settings.email;
  const std::string prev_password = settings.password;

  settings = new_settings;

  /* Invalidate token if credentials changed */
  if (new_settings.email != prev_email ||
      new_settings.password != prev_password) {
    jwt_token.clear();
    token_expiry = 0;
  }
}

bool
XCThermAPI::IsTokenValid() const {
  return !jwt_token.empty() &&
         token_expiry > static_cast<uint32_t>(std::time(nullptr));
}

bool
XCThermAPI::IsReady() const {
  return IsEnabled() &&
         (IsTokenValid() || (!settings.email.empty() && !settings.password.empty()));
}

bool
XCThermAPI::IsEnabled() const {
  return settings.enabled && !settings.email.empty() && !settings.password.empty();
}

/* ------------------------------------------------------------------ */
/* Authentication                                                      */
/* ------------------------------------------------------------------ */

bool
XCThermAPI::Authenticate() {
  LogFmt("xctherm: authenticate() user='{}'", settings.email);

  if (authenticating)
    return false;
  authenticating = true;

  const std::string post_data = R"({"email":")" + settings.email +
                                R"(","password":")" + settings.password + R"("})";

  static constexpr const char *auth_hosts[] = {
    "https://xctherm.com",
    "https://www.xctherm.com",
  };

  std::vector<uint8_t> response_buffer;
  CURLcode res = CURLE_OK;
  long http_code = 0;

  static constexpr int kMaxAttempts = 3;
  bool auth_ok = false;

  for (int attempt = 1; attempt <= kMaxAttempts && !auth_ok; ++attempt) {
    for (const char *auth_host : auth_hosts) {
      response_buffer.clear();
      http_code = 0;

      const std::string url = std::string(auth_host) + "/api/accounts/authenticate";

      CURL *curl = curl_easy_init();
      if (!curl)
        continue;

      struct curl_slist *headers = nullptr;
      struct curl_slist *resolve_list = nullptr;
      headers = curl_slist_append(headers, "Content-Type: application/json");
      AddCurlResolve(resolve_list, "xctherm.com", "138.201.15.242");
      AddCurlResolve(resolve_list, "www.xctherm.com", "138.201.15.242");

      curl_easy_setopt(curl, CURLOPT_URL, url.c_str());
      curl_easy_setopt(curl, CURLOPT_POSTFIELDS, post_data.c_str());
      curl_easy_setopt(curl, CURLOPT_HTTPHEADER, headers);
      curl_easy_setopt(curl, CURLOPT_RESOLVE, resolve_list);
      curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, CurlWriteCallback);
      curl_easy_setopt(curl, CURLOPT_WRITEDATA, &response_buffer);
      curl_easy_setopt(curl, CURLOPT_CONNECTTIMEOUT, 15L);
      curl_easy_setopt(curl, CURLOPT_TIMEOUT, 35L);
      curl_easy_setopt(curl, CURLOPT_SSL_VERIFYPEER, 0L);
      curl_easy_setopt(curl, CURLOPT_SSL_VERIFYHOST, 0L);
      curl_easy_setopt(curl, CURLOPT_NOSIGNAL, 1L);

      res = curl_easy_perform(curl);
      curl_easy_getinfo(curl, CURLINFO_RESPONSE_CODE, &http_code);

      curl_slist_free_all(headers);
      curl_slist_free_all(resolve_list);
      curl_easy_cleanup(curl);

      if (res == CURLE_OK && http_code == 200) {
        auth_ok = true;
        break;
      }

      LogFmt("xctherm: auth attempt {}/{} host={} curl={} http={}",
             attempt, kMaxAttempts, auth_host, (int)res, http_code);
    }

    if (!auth_ok && attempt < kMaxAttempts)
      std::this_thread::sleep_for(std::chrono::milliseconds(700));
  }

  authenticating = false;

  if (!auth_ok) {
    LogFmt("xctherm: auth failed curl={} http={}", (int)res, http_code);
    return false;
  }

  /* Parse JWT from JSON response (simple string search) */
  std::string response(response_buffer.begin(), response_buffer.end());

  size_t pos = response.find("\"jwtToken\":\"");
  if (pos == std::string::npos) {
    LogFmt("xctherm: jwtToken missing in response");
    return false;
  }

  pos += 12; // skip "jwtToken":"
  size_t end = response.find("\"", pos);
  if (end == std::string::npos)
    return false;

  jwt_token = response.substr(pos, end - pos);
  token_expiry = static_cast<uint32_t>(std::time(nullptr)) + 3600;

  LogFmt("xctherm: auth success, token_len={}", jwt_token.size());
  return true;
}

/* ------------------------------------------------------------------ */
/* Tile coordinate helpers                                             */
/* ------------------------------------------------------------------ */

int
XCThermAPI::LongitudeToTileX(double longitude, int zoom) {
  const double n = static_cast<double>(1 << zoom);
  int tile_x = static_cast<int>(std::floor((longitude + 180.0) / 360.0 * n));
  return std::clamp(tile_x, 0, (1 << zoom) - 1);
}

int
XCThermAPI::LatitudeToTileY(double latitude, int zoom) {
  const double clamped = std::clamp(latitude, -85.05112878, 85.05112878);
  const double lat_rad = clamped * M_PI / 180.0;
  const double n = static_cast<double>(1 << zoom);
  const double y = (1.0 - std::log(std::tan(lat_rad) + 1.0 / std::cos(lat_rad)) / M_PI) / 2.0 * n;
  return std::clamp(static_cast<int>(std::floor(y)), 0, (1 << zoom) - 1);
}

std::string
XCThermAPI::FormatDateCompact(const std::string &date) {
  std::string compact;
  compact.reserve(8);
  for (char c : date)
    if (c >= '0' && c <= '9')
      compact.push_back(c);

  if (compact.size() >= 8)
    return compact.substr(0, 8);
  return compact;
}

std::string
XCThermAPI::BuildRunCycleUtc() {
  std::time_t now = std::time(nullptr);
  std::tm *utc = std::gmtime(&now);
  if (utc == nullptr)
    return "00";

  const int run = (utc->tm_hour / 3) * 3;
  char buffer[3];
  std::snprintf(buffer, sizeof(buffer), "%02d", run);
  return std::string(buffer);
}

/* ------------------------------------------------------------------ */
/* Cache path                                                          */
/* ------------------------------------------------------------------ */

std::string
XCThermAPI::GetCachePath(const std::string &model,
                          const std::string &date_yyyymmdd,
                          const std::string &run_cycle,
                          const std::string &layer) const {
  const std::string base = DefaultCacheBasePath();
  std::string path = base + "/" + model + "/" + date_yyyymmdd + "/" +
                     run_cycle + "/" + layer;
  EnsureDirectoryRecursive(path);
  return path;
}

/* ------------------------------------------------------------------ */
/* Tile download                                                       */
/* ------------------------------------------------------------------ */

bool
XCThermAPI::DownloadTile(const std::string &model,
                          const std::string &date_yyyymmdd,
                          const std::string &run_cycle,
                          const std::string &layer,
                          int z, int x, int y,
                          std::vector<uint8_t> &out_data) {
  out_data.clear();

  /* Check cache first */
  const std::string cache_file = GetCachePath(model, date_yyyymmdd,
                                               run_cycle, layer) +
    "/" + std::to_string(z) + "_" + std::to_string(x) + "_" +
    std::to_string(y) + ".pbf";

  {
    std::ifstream cached(cache_file, std::ios::binary | std::ios::ate);
    if (cached.is_open()) {
      const auto size = cached.tellg();
      if (size > 0) {
        cached.seekg(0);
        out_data.resize(static_cast<size_t>(size));
        cached.read(reinterpret_cast<char *>(out_data.data()), size);
        if (cached.good()) {
          LogFmt("xctherm: cache hit z/x/y={}/{}/{}", z, x, y);
          return true;
        }
        out_data.clear();
      }
    }
  }

  /* Ensure valid token */
  if (!IsTokenValid() && !Authenticate())
    return false;

  static constexpr const char *hosts[] = {
    "https://t1.xctherm.com",
    "https://t2.xctherm.com",
  };

  const auto slots = BuildPreferredSlots(run_cycle);

  for (const char *slot : slots) {
    for (const char *host : hosts) {
      const std::string url = std::string(host) + "/forecast/" +
        model + "/" + date_yyyymmdd + "/" + run_cycle + "/" + slot + "/" +
        layer + "/" + std::to_string(z) + "/" +
        std::to_string(x) + "/" + std::to_string(y) + ".pbf";

      CURL *curl = curl_easy_init();
      if (!curl)
        continue;

      std::vector<uint8_t> tile_data;
      struct curl_slist *headers = nullptr;
      struct curl_slist *resolve_list = nullptr;
      const std::string auth_header = "Authorization: Bearer " + jwt_token;
      headers = curl_slist_append(headers, auth_header.c_str());
      AddCurlResolve(resolve_list, "t1.xctherm.com", "138.201.15.242");
      AddCurlResolve(resolve_list, "t2.xctherm.com", "138.201.15.242");

      curl_easy_setopt(curl, CURLOPT_URL, url.c_str());
      curl_easy_setopt(curl, CURLOPT_HTTPHEADER, headers);
      curl_easy_setopt(curl, CURLOPT_RESOLVE, resolve_list);
      curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, CurlWriteCallback);
      curl_easy_setopt(curl, CURLOPT_WRITEDATA, &tile_data);
      curl_easy_setopt(curl, CURLOPT_CONNECTTIMEOUT, 10L);
      curl_easy_setopt(curl, CURLOPT_TIMEOUT, 20L);
      curl_easy_setopt(curl, CURLOPT_FOLLOWLOCATION, 1L);
      curl_easy_setopt(curl, CURLOPT_SSL_VERIFYPEER, 0L);
      curl_easy_setopt(curl, CURLOPT_SSL_VERIFYHOST, 0L);
      curl_easy_setopt(curl, CURLOPT_NOSIGNAL, 1L);

      CURLcode res = curl_easy_perform(curl);
      long http_code = 0;
      curl_easy_getinfo(curl, CURLINFO_RESPONSE_CODE, &http_code);

      curl_slist_free_all(headers);
      curl_slist_free_all(resolve_list);
      curl_easy_cleanup(curl);

      if (http_code == 401) {
        LogFmt("xctherm: tile 401, re-auth");
        jwt_token.clear();
        token_expiry = 0;
        if (!Authenticate())
          continue;
        continue;
      }

      if (res != CURLE_OK || http_code != 200 || tile_data.empty()) {
        LogFmt("xctherm: tile miss http={} curl={}", http_code, (int)res);
        continue;
      }

      /* Write to cache */
      FILE *f = std::fopen(cache_file.c_str(), "wb");
      if (f) {
        std::fwrite(tile_data.data(), 1, tile_data.size(), f);
        std::fclose(f);
      }

      out_data = std::move(tile_data);
      return true;
    }
  }

  return false;
}

/* ------------------------------------------------------------------ */
/* FetchForecast — main entry point                                    */
/* ------------------------------------------------------------------ */

bool
XCThermAPI::FetchForecast(double latitude, double longitude,
                           const std::string &date,
                           XCThermCompositeOverlay &overlay,
                           OperationEnvironment *env,
                           uint64_t *downloaded_bytes) {
  LogFmt("xctherm: FetchForecast lat={} lon={} date={}",
         latitude, longitude, date);

  if (downloaded_bytes)
    *downloaded_bytes = 0;

  if (env) {
    env->SetText("Preparing XCTherm download...");
    env->SetProgressRange(0);
    env->SetProgressPosition(0);
  }

  const std::string compact_date = FormatDateCompact(date);
  if (compact_date.size() != 8) {
    LogFmt("xctherm: invalid date '{}'", date);
    return false;
  }

  /* Build layer name from settings */
  std::string layer;
  if (settings.use_wave_forecast)
    layer = "vertical_wind_" + std::to_string(settings.wave_height) + "amsl";
  else
    layer = "vertical_wind_" + std::to_string(settings.vertical_wind_agl) + "agl";

  /* Build run cycle candidates */
  const std::string current_run = BuildRunCycleUtc();
  int run_hour = std::clamp(std::atoi(current_run.c_str()), 0, 23);

  std::vector<std::string> run_candidates;
  for (int i = 0; i < 2; ++i) {
    int value = run_hour - i * 3;
    while (value < 0)
      value += 24;

    char buf[3];
    std::snprintf(buf, sizeof(buf), "%02d", value % 24);
    run_candidates.emplace_back(buf);
  }

  /* Find a working run by probing the center tile */
  constexpr int zoom = 11;
  const int center_x = LongitudeToTileX(longitude, zoom);
  const int center_y = LatitudeToTileY(latitude, zoom);

  std::string selected_run;
  std::vector<uint8_t> probe_data;

  for (const auto &run : run_candidates) {
    if (DownloadTile(settings.model, compact_date, run, layer,
                     zoom, center_x, center_y, probe_data)) {
      selected_run = run;
      break;
    }
  }

  if (selected_run.empty()) {
    LogFmt("xctherm: no valid run found for center tile");
    return false;
  }

  LogFmt("xctherm: selected run={} layer={}", selected_run, layer);

  /* Parse and add center tile */
  overlay.Clear();

  XCThermMVT::Tile center_tile;
  if (XCThermMVT::Parse(probe_data, center_tile)) {
    overlay.AddTile({static_cast<uint16_t>(zoom),
                     static_cast<uint16_t>(center_x),
                     static_cast<uint16_t>(center_y)},
                    std::move(center_tile));
  }

  uint64_t total_bytes = probe_data.size();

  /* Download surrounding tiles */
  static constexpr int tile_radius = 4;
  const unsigned total_tiles = (tile_radius * 2 + 1) * (tile_radius * 2 + 1);
  unsigned processed = 1; // center already done

  if (env) {
    env->SetProgressRange(total_tiles);
    env->SetProgressPosition(1);
  }

  for (int dy = -tile_radius; dy <= tile_radius; ++dy) {
    for (int dx = -tile_radius; dx <= tile_radius; ++dx) {
      if (dx == 0 && dy == 0)
        continue; // already done

      const int tile_x = center_x + dx;
      const int tile_y = center_y + dy;
      if (tile_x < 0 || tile_y < 0)
        continue;

      std::vector<uint8_t> data;
      if (DownloadTile(settings.model, compact_date, selected_run,
                       layer, zoom, tile_x, tile_y, data)) {
        total_bytes += data.size();

        XCThermMVT::Tile parsed;
        if (XCThermMVT::Parse(data, parsed)) {
          overlay.AddTile({static_cast<uint16_t>(zoom),
                           static_cast<uint16_t>(tile_x),
                           static_cast<uint16_t>(tile_y)},
                          std::move(parsed));
        }
      }

      ++processed;

      if (env) {
        env->SetProgressPosition(processed);
        const double mb = double(total_bytes) / (1024.0 * 1024.0);
        char text[192];
        std::snprintf(text, sizeof(text),
                      "Downloading wave: %u/%u tiles (%.2f MB)",
                      processed, total_tiles, mb);
        env->SetText(text);
      }
    }
  }

  if (downloaded_bytes)
    *downloaded_bytes = total_bytes;

  LogFmt("xctherm: fetch complete, {} tiles loaded, {} bytes",
         overlay.GetTileCount(), total_bytes);

  return overlay.GetTileCount() > 0;
}
