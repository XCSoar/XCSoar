// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The XCSoar Project

#pragma once

#include "system/Path.hpp"
#include "ui/event/CoInjectFunction.hpp"

#include <boost/json.hpp>

#include <chrono>
#include <deque>
#include <map>
#include <memory>
#include <string>
#include <string_view>

class CurlGlobal;
class SkySightAPI;

class SkySightRequest final {
  struct FileJob {
    enum class Kind {
      Generic,
      ForecastData,
    };

    UI::CoInjectFunction<AllocatedPath> function;
    Kind kind = Kind::Generic;
    AllocatedPath path;
    std::string url;
    bool requires_auth = false;
    std::string layer_id;
    time_t forecast_time = 0;
    unsigned attempts = 0;
    bool finished = false;

    explicit FileJob(EventLoop &event_loop) noexcept
      :function(event_loop) {}
  };

  struct PendingJob {
    FileJob::Kind kind = FileJob::Kind::Generic;
    std::string key;
    std::string url;
    AllocatedPath path;
    bool requires_auth;
    std::string layer_id;
    time_t forecast_time = 0;
    time_t ready_at = 0;
    unsigned attempts = 0;

    PendingJob(std::string _key, std::string _url,
               AllocatedPath _path, bool _requires_auth) noexcept
      :key(std::move(_key)),
       url(std::move(_url)),
       path(std::move(_path)),
       requires_auth(_requires_auth) {}

    PendingJob(FileJob::Kind _kind,
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

  static constexpr unsigned MAX_ACTIVE_DOWNLOADS = 1;
  static constexpr time_t THROTTLE_RETRY_SECONDS = 30;
  static constexpr time_t ERROR_RETRY_SECONDS = 10;
  static constexpr auto LOGIN_RETRY_INTERVAL = std::chrono::seconds{30};
  static constexpr auto LOGIN_ATTEMPT_WINDOW = std::chrono::minutes{5};
  static constexpr unsigned MAX_LOGIN_ATTEMPTS_PER_WINDOW = 3;

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
  std::deque<PendingJob> pending_jobs;
  std::map<std::string, time_t> retry_after;
  std::map<std::string, unsigned> tile_http_error_count;
  std::map<std::string, unsigned> forecast_prepare_error_count;
  std::string email;
  std::string password;
  std::string api_key;
  std::string datafiles_layer_id;
  time_t datafiles_retry_at = 0;
  time_t valid_until = 0;
  std::chrono::steady_clock::time_point next_login_request =
    std::chrono::steady_clock::time_point::min();
  std::chrono::steady_clock::time_point login_attempt_window_start =
    std::chrono::steady_clock::time_point::min();
  unsigned login_attempts_in_window = 0;
  time_t throttle_until = 0;
  time_t last_throttle_notice = 0;
  bool throttle_resume_notification_pending = false;
  bool requests_suspended = false;

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
    return requests_suspended || std::time(nullptr) < throttle_until;
  }

  bool IsSuspendedForSession() const noexcept {
    return requests_suspended;
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
  DownloadDatafileResult DownloadDatafile(std::string_view layer_id,
                                          time_t forecast_time,
                                          std::string_view url, Path filename,
                                          bool high_priority = false);
  bool RequestRegions();
  bool RequestLayers(std::string_view region_id);
  bool RequestLastUpdates(std::string_view region_id);
  bool RequestDatafiles(std::string_view region_id, std::string_view layer_id,
                        time_t from_time);

private:
  void CancelAll() noexcept;
  void EnsureLoggedIn();
  void SuspendRequests(const char *reason) noexcept;
  void CleanupFinishedJobs();
  bool IsQueued(std::string_view key) const noexcept;
  bool RequeueFileJob(const FileJob &job, time_t ready_at) noexcept;
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
  bool HandleJsonRequestHttpStatus(unsigned status,
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
  void LogThrottleNotice() noexcept;
  AllocatedPath GetThrottleCachePath() const noexcept;
  void LoadThrottleState() noexcept;
  void StoreThrottleState() noexcept;
  void ClearThrottleState() noexcept;
  void SetThrottleUntil(time_t value) noexcept;
};
