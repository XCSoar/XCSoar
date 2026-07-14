// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The XCSoar Project

#include "SkySightRequest.hpp"
#include "SkySightFileDecoder.hpp"
#include "SkysightAPI.hpp"
#include "SkySightURL.hpp"
#include "Version.hpp"
#include "co/Task.hxx"
#include "json/Serialize.hxx"
#include "io/FileOutputStream.hxx"
#include "io/StringOutputStream.hxx"
#include "io/FileLineReader.hpp"
#include "lib/curl/CoStreamRequest.hxx"
#include "lib/curl/Easy.hxx"
#include "lib/curl/Setup.hxx"
#include "lib/curl/Slist.hxx"
#include "lib/fmt/ToBuffer.hxx"
#include "lib/fmt/RuntimeError.hxx"
#include "lib/curl/Global.hxx"
#include "LogFile.hpp"
#include "system/FileUtil.hpp"

#include <boost/json.hpp>

#include <algorithm>
#include <cstring>
#include <cstdlib>
#include <ctime>
#include <span>
#include <utility>

class HttpStatusError final : public std::runtime_error {
public:
  const unsigned status;

  explicit HttpStatusError(unsigned _status)
    :std::runtime_error("SkySight HTTP request failed"),
     status(_status) {}
};

[[nodiscard]] static constexpr bool
ShouldRetryHttpDownload(unsigned status, unsigned attempts) noexcept
{
  return status == 429 || (status >= 500 && attempts < 2);
}

static_assert(ShouldRetryHttpDownload(429, 99));
static_assert(ShouldRetryHttpDownload(503, 0));
static_assert(!ShouldRetryHttpDownload(503, 2));
static_assert(!ShouldRetryHttpDownload(404, 0));

static boost::json::value
ParseJsonResponse(std::string_view body, const char *context)
{
  boost::system::error_code ec;
  auto value = boost::json::parse(body, ec);
  if (!ec)
    return value;

  throw FmtRuntimeError("{} returned invalid JSON: {}",
                        context, ec.message());
}

static Co::Task<boost::json::value>
LoginTask(CurlGlobal &curl, std::string email, std::string password)
{
  const auto url = SkySightUrl::Api("auth");
  CurlEasy easy{url.c_str()};
  Curl::Setup(easy);

  CurlSlist headers;
  headers.Append("X-API-Key: XCSoar");
  headers.Append((std::string{"User-Agent: "} + XCSoar_ProductToken).c_str());
  headers.Append("Content-Type: application/json");

  easy.SetPost();
  easy.SetRequestHeaders(headers.Get());

  const boost::json::object payload{
    {"username", email},
    {"password", password},
  };

  StringOutputStream json_stream;
  Json::Serialize(json_stream, payload);
  auto json = std::move(json_stream).GetValue();
  easy.SetRequestBody(json);
  easy.SetFailOnError(false);

  StringOutputStream body_stream;
  const auto response = co_await Curl::CoStreamRequest(curl, std::move(easy), body_stream);
  auto body = std::move(body_stream).GetValue();
  if (response.status != 200 && response.status != 201)
    throw FmtRuntimeError("SkySight login failed with status {}", response.status);

  co_return ParseJsonResponse(body, "SkySight login");
}

static Co::Task<boost::json::value>
JsonTask(CurlGlobal &curl, std::string url, std::string api_key)
{
  CurlEasy easy{url.c_str()};
  Curl::Setup(easy);
  easy.SetFailOnError(false);

  CurlSlist headers;
  if (!api_key.empty()) {
    headers.Append((std::string{"X-API-Key: "} + api_key).c_str());
    headers.Append((std::string{"User-Agent: "} + XCSoar_ProductToken).c_str());
    easy.SetRequestHeaders(headers.Get());
  }

  StringOutputStream body_stream;
  const auto response = co_await Curl::CoStreamRequest(curl, std::move(easy), body_stream);
  auto body = std::move(body_stream).GetValue();
  if (response.status != 200 && response.status != 201)
    throw HttpStatusError(response.status);

  co_return ParseJsonResponse(body, "SkySight request");
}

static Co::Task<AllocatedPath>
DownloadFileTask(CurlGlobal &curl, std::string url, AllocatedPath path,
                 std::string api_key)
{
  FileOutputStream file(path);

  CurlEasy easy{url.c_str()};
  Curl::Setup(easy);
  easy.SetFailOnError(false);

  CurlSlist headers;
  if (!api_key.empty()) {
    headers.Append((std::string{"X-API-Key: "} + api_key).c_str());
    headers.Append((std::string{"User-Agent: "} + XCSoar_ProductToken).c_str());
    easy.SetRequestHeaders(headers.Get());
  }

  const auto response = co_await Curl::CoStreamRequest(curl, std::move(easy), file);
  if (response.status != 200 && response.status != 201)
    throw HttpStatusError(response.status);

  file.Commit();
  co_return path;
}

SkySightRequest::SkySightRequest(SkysightAPI &_api, CurlGlobal &_curl,
                                 Path _cache_path) noexcept
  :api(_api),
   curl(_curl),
   cache_path(_cache_path),
   login_job(curl.GetEventLoop()),
   regions_job(curl.GetEventLoop()),
   layers_job(curl.GetEventLoop()),
   last_updates_job(curl.GetEventLoop()),
   datafiles_job(curl.GetEventLoop())
{
  LoadThrottleState();
}

SkySightRequest::~SkySightRequest() noexcept
{
  CancelAll();
}

void
SkySightRequest::CancelAll() noexcept
{
  login_job.Cancel();
  regions_job.Cancel();
  layers_job.Cancel();
  last_updates_job.Cancel();
  datafiles_job.Cancel();
  login_running = false;
  regions_running = false;
  layers_running = false;
  last_updates_running = false;
  datafiles_running = false;
  datafiles_layer_id.clear();
  datafiles_retry_at = 0;

  for (auto &i : file_jobs)
    i.second->function.Cancel();

  file_jobs.clear();
  pending_jobs.clear();
  retry_after.clear();
}

void
SkySightRequest::CancelTileDownloads() noexcept
{
  for (const auto &job : pending_jobs)
    if (job.kind == FileJob::Kind::Generic)
      retry_after.erase(job.key);

  std::erase_if(pending_jobs, [](const auto &job) {
    return job.kind == FileJob::Kind::Generic;
  });

  for (auto i = file_jobs.begin(); i != file_jobs.end();) {
    if (i->second->kind != FileJob::Kind::Generic) {
      ++i;
      continue;
    }

    i->second->function.Cancel();
    retry_after.erase(i->first);
    i = file_jobs.erase(i);
  }

  tile_http_error_count.clear();
  TryPumpQueue();
}

void
SkySightRequest::Configure(std::string_view new_email, std::string_view new_password)
{
  email = std::string{new_email};
  password = std::string{new_password};
  api_key.clear();
  valid_until = 0;
  last_login_request = 0;

  CancelAll();
}

bool
SkySightRequest::IsLoggedIn() const noexcept
{
  return !api_key.empty() && valid_until > std::time(nullptr) + 120;
}

void
SkySightRequest::CleanupFinishedJobs()
{
  for (auto i = file_jobs.begin(); i != file_jobs.end();) {
    if (i->second->finished)
      i = file_jobs.erase(i);
    else
      ++i;
  }
}

bool
SkySightRequest::IsQueued(std::string_view key) const noexcept
{
  return std::any_of(pending_jobs.begin(), pending_jobs.end(),
                     [key](const auto &job) {
                       return job.key == key;
                     });
}

bool
SkySightRequest::RequeueFileJob(const FileJob &job, time_t ready_at) noexcept
{
  try {
    PendingJob pending{
      job.kind,
      std::string{job.path.c_str()},
      job.url,
      AllocatedPath{job.path.c_str()},
      job.requires_auth,
      job.layer_id,
      job.forecast_time,
    };
    pending.ready_at = ready_at;
    pending.attempts = job.attempts + 1;
    pending_jobs.push_back(std::move(pending));
    return true;
  } catch (...) {
    LogError(std::current_exception(), "SkySight retry scheduling failed");
    return false;
  }
}

bool
SkySightRequest::Poll() noexcept
{
  TryPumpQueue();

  if (throttle_until == 0 && throttle_resume_notification_pending) {
    throttle_resume_notification_pending = false;
    return true;
  }

  return false;
}

void
SkySightRequest::PumpQueue()
{
  CleanupFinishedJobs();

  const auto now = std::time(nullptr);
  if (now < throttle_until)
    return;

  if (throttle_until != 0)
    SetThrottleUntil(0);

  while (file_jobs.size() < MAX_ACTIVE_DOWNLOADS && !pending_jobs.empty()) {
    const auto next = std::find_if(pending_jobs.begin(), pending_jobs.end(),
                                   [this, now](const auto &job) {
                                     return now >= job.ready_at &&
                                       (!job.requires_auth || IsLoggedIn());
                                   });
    if (next == pending_jobs.end()) {
      if (std::any_of(pending_jobs.begin(), pending_jobs.end(),
                      [now](const auto &job) {
                        return now >= job.ready_at && job.requires_auth;
                      }))
        EnsureLoggedIn();
      break;
    }

    auto job = std::move(*next);
    pending_jobs.erase(next);

    auto active_job = std::make_unique<FileJob>(curl.GetEventLoop());
    auto *job_ptr = active_job.get();
    const auto key = job.key;
    job_ptr->kind = job.kind;
    job_ptr->path = std::move(job.path);
    job_ptr->url = std::move(job.url);
    job_ptr->requires_auth = job.requires_auth;
    job_ptr->layer_id = job.layer_id;
    job_ptr->forecast_time = job.forecast_time;
    job_ptr->attempts = job.attempts;

    file_jobs.emplace(key, std::move(active_job));
    job_ptr->function.Start(
      DownloadFileTask(curl, job_ptr->url,
                       AllocatedPath(job_ptr->path.c_str()),
                       job.requires_auth ? api_key : std::string{}),
      [this, key](AllocatedPath) {
        OnFileSuccess(key);
      },
      [this, key](std::exception_ptr error) {
        OnFileError(key, std::move(error));
      });
  }
}

void
SkySightRequest::TryPumpQueue() noexcept
{
  try {
    PumpQueue();
  } catch (...) {
    LogError(std::current_exception(), "SkySight download scheduling failed");
  }
}

void
SkySightRequest::EnsureLoggedIn()
{
  if (!HasCredentials() || login_running || IsLoggedIn())
    return;

  const auto now = std::time(nullptr);
  if (last_login_request != 0 && now < last_login_request + 30)
    return;

  last_login_request = now;
  login_running = true;

  login_job.Start(LoginTask(curl, email, password),
                  [this](boost::json::value value) {
                    OnLoginSuccess(std::move(value));
                  },
                  [this](std::exception_ptr error) {
                    OnLoginError(std::move(error));
                  });
}

void
SkySightRequest::OnLoginSuccess(boost::json::value value)
{
  login_running = false;

  try {
    const auto &json = value.as_object();
    api_key = json.at("key").as_string().c_str();

    const auto &valid = json.at("valid_until");
    if (valid.is_number())
      valid_until = valid.to_number<time_t>();
    else if (valid.is_string())
      valid_until = std::strtoll(valid.as_string().c_str(), nullptr, 10);
    else
      valid_until = 0;

    if (!api_key.empty())
      api.OnAuthenticated();
  } catch (...) {
    api_key.clear();
    valid_until = 0;
    LogError(std::current_exception(), "SkySight login response parsing failed");
  }

  PumpQueue();
}

void
SkySightRequest::OnLoginError(std::exception_ptr error) noexcept
{
  login_running = false;
  api_key.clear();
  valid_until = 0;
  LogError(error, "SkySight login failed");
}

void
SkySightRequest::DownloadFile(std::string_view url, Path filename, bool requires_auth)
{
  PumpQueue();

  const std::string key{filename.c_str()};
  if (file_jobs.find(key) != file_jobs.end() || IsQueued(key))
    return;

  const auto now = std::time(nullptr);
  if (now < throttle_until)
    return;

  if (auto retry = retry_after.find(key); retry != retry_after.end()) {
    if (now < retry->second)
      return;

    retry_after.erase(retry);
  }

  pending_jobs.emplace_back(key, std::string{url},
                            AllocatedPath(filename.c_str()), requires_auth);
  PumpQueue();
}

SkySightRequest::DownloadDatafileResult
SkySightRequest::DownloadDatafile(std::string_view layer_id,
                                  time_t forecast_time,
                                  std::string_view url,
                                  Path filename,
                                  bool high_priority)
{
  PumpQueue();

  const std::string key{filename.c_str()};
  if (file_jobs.find(key) != file_jobs.end() || IsQueued(key))
    return DownloadDatafileResult::Duplicate;

  if (auto display_path = SkySightFileDecoder::FindCachedDisplay(filename);
      display_path != nullptr) {
    api.OnDatafileDownloaded(layer_id, forecast_time, SkySightPreparedData{
      SkySightPreparedDataKind::DisplayReady,
      {},
      std::move(display_path),
    });
    return DownloadDatafileResult::Available;
  }

  if (File::Exists(filename)) {
    if (auto retry = retry_after.find(key); retry != retry_after.end()) {
      if (std::time(nullptr) < retry->second)
        return DownloadDatafileResult::Duplicate;

      retry_after.erase(retry);
    }

    try {
      auto prepared = SkySightFileDecoder::Prepare(filename);
      api.OnDatafileDownloaded(layer_id, forecast_time,
                               std::move(prepared));
      return DownloadDatafileResult::Available;
    } catch (...) {
      LogForecastPreparationError(layer_id, forecast_time,
                                  std::current_exception());
      SkySightFileDecoder::InvalidateCache(filename);
      retry_after[key] = std::time(nullptr) + ERROR_RETRY_SECONDS;
      api.OnDatafileError(layer_id, forecast_time, true);
      return DownloadDatafileResult::Available;
    }
  }

  PendingJob job{FileJob::Kind::ForecastData,
                 key, std::string{url},
                 AllocatedPath(filename.c_str()), true,
                 std::string{layer_id}, forecast_time};
  if (high_priority)
    pending_jobs.push_front(std::move(job));
  else
    pending_jobs.push_back(std::move(job));

  PumpQueue();
  return DownloadDatafileResult::Queued;
}

bool
SkySightRequest::RequestRegions()
{
  if (regions_running)
    return false;

  if (!HasCredentials())
    return false;

  if (!IsLoggedIn()) {
    EnsureLoggedIn();
    return false;
  }

  if (std::time(nullptr) < throttle_until)
    return false;

  regions_running = true;
  regions_job.Start(
    JsonTask(curl, SkySightUrl::Api("regions"), api_key),
    [this](boost::json::value value) {
      OnRegionsSuccess(std::move(value));
    },
    [this](std::exception_ptr error) {
      OnRegionsError(std::move(error));
    });

  return true;
}

void
SkySightRequest::OnRegionsSuccess(boost::json::value value)
{
  regions_running = false;
  api.OnRegions(std::move(value));
}

void
SkySightRequest::OnRegionsError(std::exception_ptr error) noexcept
{
  regions_running = false;

  try {
    std::rethrow_exception(error);
  } catch (const HttpStatusError &http_error) {
    if (HandleJsonRequestHttpStatus(http_error.status,
                                    "SkySight regions request failed"))
      return;
  } catch (...) {
    LogError(error, "SkySight regions request failed");
  }
}

bool
SkySightRequest::RequestLayers(std::string_view region_id)
{
  if (region_id.empty() || layers_running)
    return false;

  if (!HasCredentials())
    return false;

  if (!IsLoggedIn()) {
    EnsureLoggedIn();
    return false;
  }

  if (std::time(nullptr) < throttle_until)
    return false;

  layers_running = true;

  auto url = SkySightUrl::Api("layers");
  url += "?region_id=";
  url += region_id;

  layers_job.Start(
    JsonTask(curl, std::move(url), api_key),
    [this](boost::json::value value) {
      OnLayersSuccess(std::move(value));
    },
    [this](std::exception_ptr error) {
      OnLayersError(std::move(error));
    });

  return true;
}

void
SkySightRequest::OnLayersSuccess(boost::json::value value)
{
  layers_running = false;
  api.OnLayers(std::move(value));
}

void
SkySightRequest::OnLayersError(std::exception_ptr error) noexcept
{
  layers_running = false;

  try {
    std::rethrow_exception(error);
  } catch (const HttpStatusError &http_error) {
    if (HandleJsonRequestHttpStatus(http_error.status,
                                    "SkySight layers request failed"))
      return;
  } catch (...) {
    LogError(error, "SkySight layers request failed");
  }
}

bool
SkySightRequest::RequestLastUpdates(std::string_view region_id)
{
  if (region_id.empty() || last_updates_running)
    return false;

  if (!HasCredentials())
    return false;

  if (!IsLoggedIn()) {
    EnsureLoggedIn();
    return false;
  }

  if (std::time(nullptr) < throttle_until)
    return false;

  last_updates_running = true;

  auto url = SkySightUrl::Api("data/last_updated");
  url += "?region_id=";
  url += region_id;

  last_updates_job.Start(
    JsonTask(curl, std::move(url), api_key),
    [this](boost::json::value value) {
      OnLastUpdatesSuccess(std::move(value));
    },
    [this](std::exception_ptr error) {
      OnLastUpdatesError(std::move(error));
    });

  return true;
}

bool
SkySightRequest::RequestDatafiles(std::string_view region_id,
                                  std::string_view layer_id,
                                  time_t from_time)
{
  if (region_id.empty() || layer_id.empty() || datafiles_running)
    return false;

  if (!HasCredentials())
    return false;

  if (!IsLoggedIn()) {
    EnsureLoggedIn();
    return false;
  }

  if (std::time(nullptr) < throttle_until)
    return false;

  if (std::time(nullptr) < datafiles_retry_at)
    return false;

  datafiles_running = true;
  datafiles_retry_at = 0;
  datafiles_layer_id = std::string{layer_id};

  auto url = SkySightUrl::Api("data");
  url += "?region_id=";
  url += region_id;
  url += "&layer_ids=";
  url += layer_id;
  url += "&from_time=";
  url += std::to_string(from_time);

  datafiles_job.Start(
    JsonTask(curl, std::move(url), api_key),
    [this](boost::json::value value) {
      OnDatafilesSuccess(std::move(value));
    },
    [this](std::exception_ptr error) {
      OnDatafilesError(std::move(error));
    });

  return true;
}

void
SkySightRequest::OnLastUpdatesSuccess(boost::json::value value)
{
  last_updates_running = false;
  api.OnLastUpdates(std::move(value));
}

void
SkySightRequest::OnLastUpdatesError(std::exception_ptr error) noexcept
{
  last_updates_running = false;

  try {
    std::rethrow_exception(error);
  } catch (const HttpStatusError &http_error) {
    if (HandleJsonRequestHttpStatus(http_error.status,
                                    "SkySight last-updated request failed"))
      return;
  } catch (...) {
    LogError(error, "SkySight last-updated request failed");
  }
}

void
SkySightRequest::OnDatafilesSuccess(boost::json::value value)
{
  datafiles_running = false;
  datafiles_retry_at = 0;

  const auto layer_id = std::exchange(datafiles_layer_id, std::string{});
  api.OnDatafiles(layer_id, std::move(value));
}

void
SkySightRequest::OnDatafilesError(std::exception_ptr error) noexcept
{
  datafiles_running = false;

  const auto layer_id = std::exchange(datafiles_layer_id, std::string{});

  try {
    std::rethrow_exception(error);
  } catch (const HttpStatusError &http_error) {
    if (HandleJsonRequestHttpStatus(http_error.status,
                                    "SkySight datafiles request failed"))
      return;
  } catch (...) {
    LogError(error, "SkySight datafiles request failed");
    datafiles_retry_at = std::time(nullptr) + THROTTLE_RETRY_SECONDS;
    LogFmt("SkySight forecast-step request will retry in {} seconds",
           THROTTLE_RETRY_SECONDS);
    api.OnDatafilesRetry(layer_id);
    return;
  }

  api.OnDatafilesError(layer_id);
}

bool
SkySightRequest::HandleJsonRequestHttpStatus(unsigned status,
                                             const char *context) noexcept
{
  if (status == 401 || status == 403) {
    api_key.clear();
    valid_until = 0;
  }

  if (status == 429) {
    SetThrottleUntil(std::time(nullptr) + THROTTLE_RETRY_SECONDS);
    api.OnThrottle();
    LogThrottleNotice();
    return true;
  }

  LogFmt("{} with HTTP {}", context, status);
  return false;
}

void
SkySightRequest::OnFileSuccess(const std::string &key) noexcept
{
  if (auto i = file_jobs.find(key); i != file_jobs.end()) {
    i->second->finished = true;
    const auto layer_id = i->second->layer_id;
    const auto forecast_time = i->second->forecast_time;

    try {
      switch (i->second->kind) {
      case FileJob::Kind::Generic:
        api.OnDownloadComplete();
        break;

      case FileJob::Kind::ForecastData: {
        auto prepared = SkySightFileDecoder::Prepare(i->second->path);
        api.OnDatafileDownloaded(i->second->layer_id,
                                 i->second->forecast_time,
                                 std::move(prepared));
        break;
      }
      }
    } catch (...) {
      if (i->second->kind == FileJob::Kind::ForecastData)
        SkySightFileDecoder::InvalidateCache(i->second->path);

      retry_after[key] = std::time(nullptr) + ERROR_RETRY_SECONDS;
      LogForecastPreparationError(layer_id, forecast_time,
                                  std::current_exception());
      api.OnDatafileError(layer_id, forecast_time, true);
    }
  }
  TryPumpQueue();
}

void
SkySightRequest::OnFileError(const std::string &key,
                             std::exception_ptr error) noexcept
{
  std::string layer_id;
  time_t forecast_time = 0;
  FileJob::Kind kind = FileJob::Kind::Generic;
  FileJob *failed_job = nullptr;
  if (auto i = file_jobs.find(key); i != file_jobs.end()) {
    i->second->finished = true;
    failed_job = i->second.get();
    layer_id = i->second->layer_id;
    forecast_time = i->second->forecast_time;
    kind = i->second->kind;
  }

  bool terminal_forecast_error = !layer_id.empty() &&
    kind == FileJob::Kind::ForecastData;

  try {
    std::rethrow_exception(error);
  } catch (const HttpStatusError &http_error) {
    const auto retry_delay = http_error.status == 429
      ? THROTTLE_RETRY_SECONDS
      : ERROR_RETRY_SECONDS;
    const auto retry_time = std::time(nullptr) + retry_delay;
    if (http_error.status == 429) {
      SetThrottleUntil(retry_time);
      api.OnThrottle();
      LogThrottleNotice();
      if (failed_job != nullptr) {
        terminal_forecast_error =
          !RequeueFileJob(*failed_job, retry_time);
      }
    } else if (failed_job != nullptr &&
               ShouldRetryHttpDownload(http_error.status,
                                       failed_job->attempts)) {
      terminal_forecast_error =
        !RequeueFileJob(*failed_job, retry_time);
    } else {
      retry_after[key] = retry_time;
      LogDownloadHttpError(kind == FileJob::Kind::ForecastData,
                           layer_id, forecast_time,
                           http_error.status, key);
    }
  } catch (...) {
    const auto retry_time = std::time(nullptr) + ERROR_RETRY_SECONDS;
    if (failed_job != nullptr && failed_job->attempts < 2) {
      terminal_forecast_error =
        !RequeueFileJob(*failed_job, retry_time);
    } else {
      retry_after[key] = retry_time;
    }
    if (!layer_id.empty()) {
      LogFmt("SkySight {} download failed for layer '{}' (forecast_time={})",
             kind == FileJob::Kind::ForecastData ? "forecast" : "tile",
             layer_id, (long long)forecast_time);
    }
    LogError(error,
             kind == FileJob::Kind::ForecastData
             ? "SkySight forecast download failed"
             : "SkySight tile download failed");
  }

  if (terminal_forecast_error)
    api.OnDatafileError(layer_id, forecast_time);

  TryPumpQueue();
}

AllocatedPath
SkySightRequest::GetThrottleCachePath() const noexcept
{
  return AllocatedPath::Build(cache_path, "throttle-v1.cache");
}

void
SkySightRequest::LoadThrottleState() noexcept
{
  const auto path = GetThrottleCachePath();

  try {
    FileLineReaderA reader(path);
    const char *line = reader.ReadLine();
    if (line == nullptr || *line == 0) {
      ClearThrottleState();
      return;
    }

    const auto persisted_until = std::strtoll(line, nullptr, 10);
    const auto now = std::time(nullptr);
    if (persisted_until <= now) {
      ClearThrottleState();
      return;
    }

    throttle_until = persisted_until;
  } catch (...) {
    ClearThrottleState();
  }
}

void
SkySightRequest::StoreThrottleState() noexcept
{
  if (throttle_until <= std::time(nullptr)) {
    ClearThrottleState();
    return;
  }

  try {
    FileOutputStream file{GetThrottleCachePath()};
    const auto buffer = FmtBuffer<32>("{}\n", (long long)throttle_until);
    file.Write(std::as_bytes(std::span{buffer.c_str(), std::strlen(buffer.c_str())}));
    file.Commit();
  } catch (...) {
  }
}

void
SkySightRequest::ClearThrottleState() noexcept
{
  throttle_until = 0;
  File::Delete(GetThrottleCachePath());
}

void
SkySightRequest::SetThrottleUntil(time_t value) noexcept
{
  throttle_until = value;
  if (throttle_until > std::time(nullptr)) {
    throttle_resume_notification_pending = true;
    StoreThrottleState();
  } else
    ClearThrottleState();
}

void
SkySightRequest::LogThrottleNotice() noexcept
{
  const auto now = std::time(nullptr);
  if (last_throttle_notice != 0 &&
      now < last_throttle_notice + THROTTLE_RETRY_SECONDS)
    return;

  last_throttle_notice = now;
  LogFmt("SkySight throttled by server (HTTP 429), pausing requests for {} seconds",
         unsigned(THROTTLE_RETRY_SECONDS));
}

void
SkySightRequest::LogForecastPreparationError(std::string_view layer_id,
                                             time_t forecast_time,
                                             std::exception_ptr error) noexcept
{
  if (layer_id.empty()) {
    LogError(error, "SkySight forecast file preparation failed");
    return;
  }

  auto &count = forecast_prepare_error_count[std::string{layer_id}];
  ++count;

  if (count == 1) {
    LogFmt("SkySight forecast file preparation failed for layer '{}' (forecast_time={}); suppressing repeats",
           layer_id, (long long)forecast_time);
    LogError(error, "SkySight forecast file preparation failed");
    return;
  }

  if ((count % 20) == 0) {
    LogFmt("SkySight forecast file preparation still failing for layer '{}' ({} failures)",
           layer_id, count);
    LogError(error, "SkySight forecast file preparation failed");
  }
}

void
SkySightRequest::LogDownloadHttpError(bool forecast_download,
                                      std::string_view layer_id,
                                      time_t forecast_time,
                                      unsigned status,
                                      std::string_view key) noexcept
{
  const auto bucket = FmtBuffer<64>("{}|{}|{}",
                                    forecast_download ? "forecast" : "tile",
                                    layer_id.empty() ? "*" : layer_id,
                                    status);
  auto &count = tile_http_error_count[std::string{bucket.c_str()}];
  ++count;

  if (count == 1) {
    if (!layer_id.empty()) {
      LogFmt("SkySight {} download failed with HTTP {} for layer '{}' (forecast_time={}); suppressing repeats",
             forecast_download ? "forecast" : "tile",
             status, layer_id, (long long)forecast_time);
    } else {
      LogFmt("SkySight {} download failed with HTTP {} (request='{}'); suppressing repeats",
             forecast_download ? "forecast" : "tile",
             status, key);
    }
    return;
  }

  if ((count % 20) == 0) {
    if (!layer_id.empty()) {
      LogFmt("SkySight {} download still failing with HTTP {} for layer '{}' ({} failures)",
             forecast_download ? "forecast" : "tile",
             status, layer_id, count);
    } else {
      LogFmt("SkySight {} download still failing with HTTP {} ({} failures)",
             forecast_download ? "forecast" : "tile",
             status, count);
    }
  }
}
