// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The XCSoar Project

#pragma once

#include "system/Path.hpp"
#include "ui/event/CoInjectFunction.hpp"
#include "SkySightRequestPolicy.hpp"

#include <boost/json.hpp>

#include <deque>
#include <map>
#include <memory>
#include <set>
#include <string>
#include <string_view>

class CurlGlobal;
class SkySightAPI;

class SkySightRequest final {
  struct FileRequest {
    enum class Kind {
      Generic,
      ForecastData,
    };

    Kind kind = Kind::Generic;
    std::string key;
    std::string url;
    AllocatedPath path;
    bool requires_auth = false;
    std::string layer_id;
    time_t forecast_time = 0;
    time_t ready_at = 0;
    unsigned attempts = 0;

    FileRequest(std::string _key, std::string _url,
               AllocatedPath _path, bool _requires_auth) noexcept
      :key(std::move(_key)),
       url(std::move(_url)),
       path(std::move(_path)),
       requires_auth(_requires_auth) {}

    FileRequest(Kind _kind,
               std::string _key, std::string _url,
               AllocatedPath _path, bool _requires_auth,
               std::string _layer_id, time_t _forecast_time) noexcept
      :kind(_kind),
       key(std::move(_key)),
       url(std::move(_url)),
       path(std::move(_path)),
       requires_auth(_requires_auth),
       layer_id(std::move(_layer_id)),
       forecast_time(_forecast_time) {}
  };

  struct FileJob final : FileRequest {
    UI::CoInjectFunction<AllocatedPath> function;
    bool finished = false;

    FileJob(EventLoop &event_loop, FileRequest request) noexcept
      :FileRequest(std::move(request)), function(event_loop) {}
  };

  static constexpr unsigned MAX_ACTIVE_DOWNLOADS = 1;
  static constexpr time_t THROTTLE_RETRY_SECONDS = 30;
  static constexpr time_t ERROR_RETRY_SECONDS = 10;

  SkySightAPI &api;
  CurlGlobal &curl;
  const AllocatedPath cache_path;
  UI::CoInjectFunction<boost::json::value> login_job;
  UI::CoInjectFunction<boost::json::value> regions_job;
  UI::CoInjectFunction<boost::json::value> layers_job;
  UI::CoInjectFunction<boost::json::value> last_updates_job;
  UI::CoInjectFunction<boost::json::value> datafiles_job;
  bool login_running = false;
  bool regions_running = false;
  bool layers_running = false;
  bool last_updates_running = false;
  bool datafiles_running = false;
  std::map<std::string, std::unique_ptr<FileJob>> file_jobs;
  std::deque<FileRequest> pending_jobs;
  SkySight::AuthenticationFailurePolicy authentication_failures;
  SkySight::RequestFailurePolicy download_failures;
  SkySight::LiveTileRequestPacer live_tile_pacer;
  std::map<std::string, time_t> payload_retry_at;
  std::set<std::string, std::less<>> generic_keys;
  std::map<std::string, unsigned> tile_http_error_count;
  std::map<std::string, unsigned> forecast_prepare_error_count;
  std::string email;
  std::string password;
  std::string api_key;
  std::string last_updates_layer_id;
  std::string datafiles_layer_id;
  time_t datafiles_retry_at = 0;
  time_t valid_until = 0;
  time_t last_login_request = 0;
  time_t throttle_until = 0;
  time_t last_throttle_notice = 0;
  bool throttle_resume_notification_pending = false;

public:
  enum class DownloadDatafileResult {
    Duplicate,
    Queued,
    Available,
  };

  SkySightRequest(SkySightAPI &_api, CurlGlobal &_curl, Path _cache_path) noexcept;
  ~SkySightRequest() noexcept;

  void Configure(std::string_view new_email, std::string_view new_password);

  bool HasCredentials() const noexcept {
    return !email.empty() && !password.empty();
  }

  bool IsLoggedIn() const noexcept;

  bool IsThrottled() const noexcept {
    return std::time(nullptr) < throttle_until;
  }

  time_t GetThrottleRemainingSeconds() const noexcept {
    const auto now = std::time(nullptr);
    return throttle_until > now ? throttle_until - now : 0;
  }

  time_t GetDatafilesRetryRemainingSeconds() const noexcept {
    const auto now = std::time(nullptr);
    return datafiles_retry_at > now ? datafiles_retry_at - now : 0;
  }

  /** Pump deferred downloads and report when a throttle pause has ended. */
  bool Poll() noexcept;

  void DownloadFile(std::string_view url, Path filename, bool requires_auth);
  void CancelTileDownloads() noexcept;
  void ReconcileTileDownloads(
    const std::set<std::string, std::less<>> &desired_keys) noexcept;
  [[nodiscard]] bool HasPendingTileDownloads() const noexcept;
  DownloadDatafileResult DownloadDatafile(std::string_view layer_id,
                                          time_t forecast_time,
                                          std::string_view url, Path filename,
                                          bool high_priority = false);
  void SuppressDatafile(Path filename) noexcept;
  bool RequestRegions();
  bool RequestLayers(std::string_view region_id);
  bool RequestLastUpdates(std::string_view region_id,
                          std::string_view layer_id);
  bool RequestDatafiles(std::string_view region_id, std::string_view layer_id,
                        time_t from_time);

private:
  void CancelAll() noexcept;
  void EnsureLoggedIn();
  void CleanupFinishedJobs();
  bool IsQueued(std::string_view key) const noexcept;
  bool RequeueFileJob(FileJob &job, time_t ready_at) noexcept;
  void PumpQueue();
  void TryPumpQueue() noexcept;
  void OnLoginSuccess(boost::json::value value);
  void OnLoginError(std::exception_ptr error) noexcept;
  void OnRegionsSuccess(boost::json::value value);
  void OnRegionsError(std::exception_ptr error) noexcept;
  void OnLayersSuccess(boost::json::value value);
  void OnLayersError(std::exception_ptr error) noexcept;
  void OnLastUpdatesSuccess(boost::json::value value);
  void OnLastUpdatesError(std::exception_ptr error) noexcept;
  void OnDatafilesSuccess(boost::json::value value);
  void OnDatafilesError(std::exception_ptr error) noexcept;
  bool HandleJsonRequestHttpStatus(unsigned status, time_t retry_at,
                                   const char *context) noexcept;
  void OnFileSuccess(const std::string &key) noexcept;
  void OnFileError(const std::string &key, std::exception_ptr error) noexcept;
  void LogForecastPreparationError(std::string_view layer_id,
                                   time_t forecast_time,
                                   std::exception_ptr error) noexcept;
  void LogDownloadHttpError(bool forecast_download,
                            std::string_view layer_id,
                            time_t forecast_time,
                            unsigned status,
                            std::string_view key) noexcept;
  void LogThrottleNotice(bool server_retry_after) noexcept;
  AllocatedPath GetThrottleCachePath() const noexcept;
  void LoadThrottleState() noexcept;
  void StoreThrottleState() noexcept;
  void ClearThrottleState() noexcept;
  void SetThrottleUntil(time_t value) noexcept;
};
