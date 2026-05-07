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
    UI::CoInjectFunction<AllocatedPath> function;
    bool finished = false;

    explicit FileJob(EventLoop &event_loop) noexcept
      :function(event_loop) {}
  };

  struct PendingJob {
    std::string key;
    std::string url;
    AllocatedPath path;
    bool requires_auth;

      PendingJob(std::string _key, std::string _url,
        AllocatedPath _path, bool _requires_auth) noexcept
     :key(std::move(_key)),
      url(std::move(_url)),
      path(std::move(_path)),
       requires_auth(_requires_auth) {}
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
  bool login_running = false;
  bool regions_running = false;
  bool layers_running = false;
  bool last_updates_running = false;
  std::map<std::string, std::unique_ptr<FileJob>> file_jobs;
  std::deque<PendingJob> pending_jobs;
  std::map<std::string, time_t> retry_after;
  std::string email;
  std::string password;
  std::string api_key;
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
  void RequestRegions();
  void RequestLayers(std::string_view region_id);
  void RequestLastUpdates(std::string_view region_id);

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
  void OnFileSuccess(const std::string &key) noexcept;
  void OnFileError(const std::string &key, std::exception_ptr error) noexcept;
};