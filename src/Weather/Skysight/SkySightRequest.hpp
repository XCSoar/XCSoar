// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The XCSoar Project

#pragma once

#include "system/Path.hpp"
#include "ui/event/CoInjectFunction.hpp"

#include <boost/json.hpp>

#include <deque>
#include <map>
#include <memory>
#include <string>
#include <string_view>

class CurlGlobal;
class SkysightAPI;

class SkySightRequest final {
  struct FileJob {
    enum class Kind {
      Generic,
      ForecastData,
    };

    UI::CoInjectFunction<AllocatedPath> function;
    Kind kind = Kind::Generic;
    AllocatedPath path;
    std::string layer_id;
    time_t forecast_time = 0;
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

  SkysightAPI &api;
  CurlGlobal &curl;
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
  std::string email;
  std::string password;
  std::string api_key;
  std::string datafiles_layer_id;
  time_t valid_until = 0;
  time_t last_login_request = 0;
  time_t throttle_until = 0;

public:
  SkySightRequest(SkysightAPI &_api, CurlGlobal &_curl) noexcept;
  ~SkySightRequest() noexcept;

  void Configure(std::string_view new_email, std::string_view new_password);

  bool HasCredentials() const noexcept {
    return !email.empty() && !password.empty();
  }

  bool IsLoggedIn() const noexcept;

  void DownloadFile(std::string_view url, Path filename, bool requires_auth);
  void DownloadDatafile(std::string_view layer_id, time_t forecast_time,
                        std::string_view url, Path filename);
  void RequestRegions();
  void RequestLayers(std::string_view region_id);
  void RequestLastUpdates(std::string_view region_id);
  void RequestDatafiles(std::string_view region_id, std::string_view layer_id,
                        time_t from_time);

private:
  void CancelAll() noexcept;
  void EnsureLoggedIn();
  void CleanupFinishedJobs();
  bool IsQueued(std::string_view key) const noexcept;
  void PumpQueue();
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
  void OnFileSuccess(const std::string &key) noexcept;
  void OnFileError(const std::string &key, std::exception_ptr error) noexcept;
};