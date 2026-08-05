// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The XCSoar Project

#include "SkySightRequest.hpp"
#include "SkySightCache.hpp"
#include "SkySightFileDecoder.hpp"
#include "SkySightLimits.hpp"
#include "SkySightAPI.hpp"
#include "SkySightURL.hpp"
#include "Version.hpp"
#include "co/Task.hxx"
#include "json/Serialize.hxx"
#include "io/FileOutputStream.hxx"
#include "io/StringOutputStream.hxx"
#include "io/FileLineReader.hpp"
#include "lib/curl/CoStreamRequest.hxx"
#include "lib/curl/Easy.hxx"
#include "lib/curl/Error.hxx"
#include "lib/curl/Setup.hxx"
#include "lib/curl/Slist.hxx"
#include "lib/fmt/ToBuffer.hxx"
#include "lib/fmt/RuntimeError.hxx"
#include "lib/curl/Global.hxx"
#include "LogFile.hpp"
#include "system/FileUtil.hpp"

#include <boost/json.hpp>

#include <algorithm>
#include <cstddef>
#include <cstring>
#include <cstdlib>
#include <ctime>
#include <limits>
#include <span>
#include <utility>

namespace {

[[nodiscard]] bool
IsHostResolutionFailure(std::exception_ptr error) noexcept
{
  if (error == nullptr)
    return false;

  try {
    std::rethrow_exception(error);
  } catch (const std::system_error &e) {
    return e.code().category() == Curl::error_category &&
      e.code().value() == CURLE_COULDNT_RESOLVE_HOST;
  } catch (...) {
    return false;
  }
}

class LimitedOutputStream final : public OutputStream {
  OutputStream &destination;
  std::size_t remaining;

public:
  LimitedOutputStream(OutputStream &_destination, std::size_t maximum) noexcept
    :destination(_destination), remaining(maximum) {}

  void Write(std::span<const std::byte> source) override {
    if (source.size() > remaining)
      throw SkySight::ResourceLimitError(
        "SkySight response exceeds its size limit");

    destination.Write(source);
    remaining -= source.size();
  }
};

void
ConfigureSkySightTransfer(CurlEasy &easy, long timeout_seconds)
{
  easy.SetTimeout(timeout_seconds);
  easy.SetOption(CURLOPT_LOW_SPEED_LIMIT, 128L);
  easy.SetOption(CURLOPT_LOW_SPEED_TIME, 60L);
}

} // namespace

class HttpStatusError final : public std::runtime_error {
public:
  const unsigned status;
  const time_t retry_at;

  explicit HttpStatusError(unsigned _status, time_t _retry_at=0)
    :std::runtime_error("SkySight HTTP request failed"),
     status(_status), retry_at(_retry_at) {}
};

[[nodiscard]] static time_t
ParseRetryAfter(const Curl::Headers &headers, time_t now) noexcept
{
  const auto [begin, end] = headers.equal_range("retry-after");
  for (auto i = begin; i != end; ++i) {
    const auto value = std::string_view{i->second};
    if (const auto retry_at = SkySight::ParseRetryAfterSeconds(value, now);
        retry_at != 0)
      return retry_at;

    const auto date = curl_getdate(i->second.c_str(), nullptr);
    if (date > now)
      return date;
  }

  return 0;
}
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
  ConfigureSkySightTransfer(easy, 60);

  CurlSlist headers;
  headers.Append("X-API-Key: XCSoar");
  headers.Append((std::string{"User-Agent: "} + XCSoar_ProductToken).c_str());
  headers.Append("Content-Type: application/json");

  easy.SetPost();
  easy.SetRequestHeaders(headers.Get());

  const boost::json::object payload{
    {"username", email},
    {"password", password},
    {"device_type", "XCSoar"},
    {"device_serial", 0},
  };

  StringOutputStream json_stream;
  Json::Serialize(json_stream, payload);
  auto json = std::move(json_stream).GetValue();
  easy.SetRequestBody(json);
  easy.SetFailOnError(false);

  StringOutputStream body_stream;
  LimitedOutputStream limited_body{body_stream,
                                   SkySight::MAX_JSON_RESPONSE_BYTES};
  const auto response = co_await Curl::CoStreamRequest(curl, std::move(easy),
                                                       limited_body);
  auto body = std::move(body_stream).GetValue();
  if (response.status != 200 && response.status != 201)
    throw HttpStatusError(response.status,
                          ParseRetryAfter(response.headers, std::time(nullptr)));

  co_return ParseJsonResponse(body, "SkySight login");
}

static Co::Task<boost::json::value>
JsonTask(CurlGlobal &curl, std::string url, std::string api_key)
{
  CurlEasy easy{url.c_str()};
  Curl::Setup(easy);
  ConfigureSkySightTransfer(easy, 60);
  easy.SetFailOnError(false);

  CurlSlist headers;
  if (!api_key.empty()) {
    headers.Append((std::string{"X-API-Key: "} + api_key).c_str());
    headers.Append((std::string{"User-Agent: "} + XCSoar_ProductToken).c_str());
    easy.SetRequestHeaders(headers.Get());
  }

  StringOutputStream body_stream;
  LimitedOutputStream limited_body{body_stream,
                                   SkySight::MAX_JSON_RESPONSE_BYTES};
  const auto response = co_await Curl::CoStreamRequest(curl, std::move(easy),
                                                       limited_body);
  auto body = std::move(body_stream).GetValue();
  if (response.status != 200 && response.status != 201)
    throw HttpStatusError(response.status,
                          ParseRetryAfter(response.headers, std::time(nullptr)));

  co_return ParseJsonResponse(body, "SkySight request");
}

static Co::Task<AllocatedPath>
DownloadFileTask(CurlGlobal &curl, std::string url, AllocatedPath path,
                 std::string api_key, std::size_t maximum_size,
                 long timeout_seconds)
{
  FileOutputStream file(path);
  LimitedOutputStream limited_file{file, maximum_size};

  CurlEasy easy{url.c_str()};
  Curl::Setup(easy);
  ConfigureSkySightTransfer(easy, timeout_seconds);
  easy.SetFailOnError(false);

  CurlSlist headers;
  if (!api_key.empty()) {
    headers.Append((std::string{"X-API-Key: "} + api_key).c_str());
    headers.Append((std::string{"User-Agent: "} + XCSoar_ProductToken).c_str());
    easy.SetRequestHeaders(headers.Get());
  }

  const auto response = co_await Curl::CoStreamRequest(curl, std::move(easy),
                                                       limited_file);
  if (response.status != 200 && response.status != 201)
    throw HttpStatusError(response.status,
                          ParseRetryAfter(response.headers, std::time(nullptr)));

  file.Commit();
  co_return path;
}

SkySightRequest::SkySightRequest(SkySightAPI &_api, CurlGlobal &_curl,
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
  last_updates_layer_id.clear();
  datafiles_running = false;
  datafiles_layer_id.clear();
  datafiles_retry_at = 0;

  CancelFileDownloads();
}

void
SkySightRequest::CancelFileDownloads() noexcept
{
  for (auto &i : file_jobs)
    i.second->function.Cancel();

  file_jobs.clear();
  pending_jobs.clear();
  download_failures.Clear();
  payload_retry_at.clear();
  generic_keys.clear();
  tile_http_error_count.clear();
}

void
SkySightRequest::CancelTileDownloads() noexcept
{
  for (const auto &job : pending_jobs)
    if (job.kind == FileJob::Kind::Generic)
      payload_retry_at.erase(job.key);

  std::erase_if(pending_jobs, [](const auto &job) {
    return job.kind == FileJob::Kind::Generic;
  });

  for (auto i = file_jobs.begin(); i != file_jobs.end();) {
    if (i->second->kind != FileJob::Kind::Generic) {
      ++i;
      continue;
    }

    i->second->function.Cancel();
    payload_retry_at.erase(i->first);
    i = file_jobs.erase(i);
  }

  tile_http_error_count.clear();
  for (const auto &key : generic_keys)
    download_failures.Erase(key);
  generic_keys.clear();
  TryPumpQueue();
}

void
SkySightRequest::ReconcileTileDownloads(
  const std::set<std::string, std::less<>> &desired_keys) noexcept
{
  std::erase_if(pending_jobs, [&desired_keys](const auto &job) {
    return job.kind == FileJob::Kind::Generic &&
      !desired_keys.contains(job.key);
  });

  for (auto i = file_jobs.begin(); i != file_jobs.end();) {
    if (i->second->kind != FileJob::Kind::Generic ||
        desired_keys.contains(i->first)) {
      ++i;
      continue;
    }

    i->second->function.Cancel();
    i = file_jobs.erase(i);
  }

  for (auto i = generic_keys.begin(); i != generic_keys.end();) {
    if (desired_keys.contains(*i)) {
      ++i;
      continue;
    }

    payload_retry_at.erase(*i);
    download_failures.Erase(*i);
    i = generic_keys.erase(i);
  }
}

bool
SkySightRequest::HasPendingTileDownloads() const noexcept
{
  return std::any_of(pending_jobs.begin(), pending_jobs.end(),
                     [](const auto &job) {
                       return job.kind == FileJob::Kind::Generic;
                     }) ||
    std::any_of(file_jobs.begin(), file_jobs.end(),
                [](const auto &entry) {
                  const auto &job = *entry.second;
                  return job.kind == FileJob::Kind::Generic && !job.finished;
                });
}

void
SkySightRequest::Configure(std::string_view new_email, std::string_view new_password)
{
  email = std::string{new_email};
  password = std::string{new_password};
  api_key.clear();
  valid_until = 0;
  last_login_request = 0;
  authentication_failures.Reset();

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
SkySightRequest::RequeueFileJob(FileJob &job, time_t ready_at) noexcept
{
  try {
    FileRequest pending{std::move(static_cast<FileRequest &>(job))};
    pending.ready_at = ready_at;
    pending.attempts = download_failures.GetAttempts(pending.key);
    if (pending.kind == FileJob::Kind::Generic)
      pending_jobs.push_front(std::move(pending));
    else
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
  const bool has_live_tile_work =
    std::any_of(pending_jobs.begin(), pending_jobs.end(),
                [](const auto &job) {
                  return job.kind == FileJob::Kind::Generic;
                }) ||
    std::any_of(file_jobs.begin(), file_jobs.end(),
                [](const auto &entry) {
                  return entry.second->kind == FileJob::Kind::Generic;
                });
  live_tile_pacer.OnQueueState(now, has_live_tile_work);

  if (now < throttle_until)
    return;

  if (throttle_until != 0)
    SetThrottleUntil(0);

  while (file_jobs.size() < MAX_ACTIVE_DOWNLOADS && !pending_jobs.empty()) {
    const auto next = std::find_if(pending_jobs.begin(), pending_jobs.end(),
                                   [this, now](const auto &job) {
                                     return now >= job.ready_at &&
                                       (job.kind == FileJob::Kind::ForecastData ||
                                        (interactive_request_pacer.CanStart(now) &&
                                         live_tile_pacer.CanStart(now))) &&
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

    auto active_job = std::make_unique<FileJob>(curl.GetEventLoop(),
                                                std::move(job));
    auto *job_ptr = active_job.get();
    const auto key = job_ptr->key;
    if (job_ptr->kind == FileJob::Kind::Generic) {
      interactive_request_pacer.OnStarted(now);
      live_tile_pacer.OnStarted(now);
    }

    file_jobs.emplace(key, std::move(active_job));
    const bool tile_download = job_ptr->kind == FileJob::Kind::Generic;
    try {
      job_ptr->function.Start(
        DownloadFileTask(curl, job_ptr->url,
                         AllocatedPath(job_ptr->path.c_str()),
                         job_ptr->requires_auth ? api_key : std::string{},
                         tile_download
                           ? SkySight::MAX_TILE_DOWNLOAD_BYTES
                           : SkySight::MAX_FORECAST_DOWNLOAD_BYTES,
                         tile_download ? 60 : 10 * 60),
        [this, key](AllocatedPath) {
          OnFileSuccess(key);
        },
        [this, key](std::exception_ptr error) {
          OnFileError(key, std::move(error));
        });
    } catch (...) {
      file_jobs.erase(key);
      throw;
    }
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
  if (now < throttle_until || !authentication_failures.CanAttempt(now))
    return;

  if (!interactive_request_pacer.CanStart(now))
    return;

  if (last_login_request != 0 && now < last_login_request + 30)
    return;

  last_login_request = now;
  login_running = true;
  interactive_request_pacer.OnStarted(now);

  try {
    login_job.Start(LoginTask(curl, email, password),
                    [this](boost::json::value value) {
                      OnLoginSuccess(std::move(value));
                    },
                    [this](std::exception_ptr error) {
                      OnLoginError(std::move(error));
                    });
  } catch (...) {
    login_running = false;
    throw;
  }
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

    if (api_key.empty() || valid_until <= std::time(nullptr) + 120)
      throw std::runtime_error("SkySight login returned no usable API key");

    authentication_failures.Reset();
    api.OnAuthenticated();
  } catch (...) {
    api_key.clear();
    valid_until = 0;
    authentication_failures.OnTransportFailure(std::time(nullptr));
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
  const bool host_resolution_failure = IsHostResolutionFailure(error);

  try {
    std::rethrow_exception(error);
  } catch (const SkySight::ResourceLimitError &) {
    authentication_failures.OnHttpFailure(400, std::time(nullptr));
    LogError(error, "SkySight login response exceeded its size limit");
  } catch (const HttpStatusError &http_error) {
    const auto now = std::time(nullptr);
    const auto retry_at = http_error.status == 429
      ? throttle_fallback.OnThrottle(now, http_error.retry_at)
      : http_error.retry_at;
    const auto decision = authentication_failures.OnHttpFailure(
      http_error.status, now, retry_at);
    if (decision.action == SkySight::AuthenticationFailureAction::Throttle) {
      SetThrottleUntil(decision.ready_at);
      LogFmt("SkySight throttled by server (HTTP 429), pausing requests for {} "
             "seconds ({})", unsigned(decision.ready_at - now),
             http_error.retry_at > now
             ? "server Retry-After" : "client fallback");
    } else if (decision.action ==
               SkySight::AuthenticationFailureAction::Rejected) {
      LogFmt("SkySight login rejected with HTTP {}; automatic login disabled "
             "until credentials are configured again", http_error.status);
    }
  } catch (...) {
    if (host_resolution_failure)
      authentication_failures.Reset();
    authentication_failures.OnTransportFailure(std::time(nullptr));
  }

  if (!host_resolution_failure)
    LogError(error, "SkySight login failed");
  PumpQueue();
}

void
SkySightRequest::DownloadFile(std::string_view url, Path filename, bool requires_auth)
{
  PumpQueue();

  const std::string key{filename.c_str()};
  if (file_jobs.find(key) != file_jobs.end() || IsQueued(key))
    return;

  const auto now = std::time(nullptr);
  generic_keys.insert(key);
  if (!download_failures.CanQueue(key, now))
    return;

  if (now < throttle_until)
    return;

  if (auto retry = payload_retry_at.find(key); retry != payload_retry_at.end()) {
    if (now < retry->second)
      return;

    payload_retry_at.erase(retry);
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

  if (!download_failures.CanQueue(key, std::time(nullptr)))
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

  if (auto retry = payload_retry_at.find(key); retry != payload_retry_at.end()) {
    if (std::time(nullptr) < retry->second)
      return DownloadDatafileResult::Duplicate;

    payload_retry_at.erase(retry);
  }

  if (File::Exists(filename)) {
    api.OnDatafileDownloaded(layer_id, forecast_time,
                             SkySightFileDecoder::MakeDeferredPreparation(filename));
    return DownloadDatafileResult::Available;
  }

  FileRequest job{FileJob::Kind::ForecastData,
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

void
SkySightRequest::SuppressDatafile(Path filename) noexcept
{
  try {
    payload_retry_at[filename.c_str()] = std::numeric_limits<time_t>::max();
  } catch (...) {
    LogError(std::current_exception(),
             "SkySight datafile suppression failed");
  }
}

bool
SkySightRequest::StartAuthenticatedJsonRequest(
  bool &running,
  UI::CoInjectFunction<boost::json::value> &job,
  std::string url,
  std::function<void(boost::json::value)> on_success,
  std::function<void(std::exception_ptr)> on_error,
  std::function<void()> before_start)
{
  if (running)
    return false;

  if (!HasCredentials())
    return false;

  if (!IsLoggedIn()) {
    EnsureLoggedIn();
    return false;
  }

  const auto now = std::time(nullptr);
  if (now < throttle_until)
    return false;

  if (!interactive_request_pacer.CanStart(now))
    return false;

  running = true;
  if (before_start)
    before_start();

  interactive_request_pacer.OnStarted(now);
  job.Start(JsonTask(curl, std::move(url), api_key),
            std::move(on_success),
            std::move(on_error));
  return true;
}

void
SkySightRequest::HandleAuthenticatedJsonError(std::exception_ptr error,
                                              const char *context) noexcept
{
  try {
    std::rethrow_exception(error);
  } catch (const SkySight::ResourceLimitError &) {
    LogError(error, context);
  } catch (const HttpStatusError &http_error) {
    HandleJsonRequestHttpStatus(http_error.status, http_error.retry_at,
                                context);
  } catch (...) {
    if (!IsHostResolutionFailure(error))
      LogError(error, context);
  }
}

bool
SkySightRequest::RequestRegions()
{
  return StartAuthenticatedJsonRequest(
    regions_running, regions_job,
    SkySightUrl::Api("regions"),
    [this](boost::json::value value) {
      OnRegionsSuccess(std::move(value));
    },
    [this](std::exception_ptr error) {
      OnRegionsError(std::move(error));
    });
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
  HandleAuthenticatedJsonError(std::move(error),
                               "SkySight regions request failed");
}

bool
SkySightRequest::RequestLayers(std::string_view region_id)
{
  if (region_id.empty())
    return false;

  auto url = SkySightUrl::Api("layers");
  url += "?region_id=";
  url += region_id;

  return StartAuthenticatedJsonRequest(
    layers_running, layers_job, std::move(url),
    [this](boost::json::value value) {
      OnLayersSuccess(std::move(value));
    },
    [this](std::exception_ptr error) {
      OnLayersError(std::move(error));
    });
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
  HandleAuthenticatedJsonError(std::move(error),
                               "SkySight layers request failed");
}

bool
SkySightRequest::RequestLastUpdates(std::string_view region_id,
                                    std::string_view layer_id)
{
  if (region_id.empty() || layer_id.empty())
    return false;

  auto url = SkySightUrl::Api("data/last_updated");
  url += "?region_id=";
  url += region_id;
  url += "&layer_ids=";
  url += layer_id;

  return StartAuthenticatedJsonRequest(
    last_updates_running, last_updates_job, std::move(url),
    [this](boost::json::value value) {
      OnLastUpdatesSuccess(std::move(value));
    },
    [this](std::exception_ptr error) {
      OnLastUpdatesError(std::move(error));
    },
    [this, id = std::string{layer_id}]() {
      last_updates_layer_id = id;
    });
}

bool
SkySightRequest::RequestDatafiles(std::string_view region_id,
                                  std::string_view layer_id,
                                  time_t from_time)
{
  if (region_id.empty() || layer_id.empty())
    return false;

  if (std::time(nullptr) < datafiles_retry_at)
    return false;

  auto url = SkySightUrl::Api("data");
  url += "?region_id=";
  url += region_id;
  url += "&layer_ids=";
  url += layer_id;
  url += "&from_time=";
  url += std::to_string(from_time);

  return StartAuthenticatedJsonRequest(
    datafiles_running, datafiles_job, std::move(url),
    [this](boost::json::value value) {
      OnDatafilesSuccess(std::move(value));
    },
    [this](std::exception_ptr error) {
      OnDatafilesError(std::move(error));
    },
    [this, id = std::string{layer_id}]() {
      datafiles_retry_at = 0;
      datafiles_layer_id = id;
    });
}

void
SkySightRequest::OnLastUpdatesSuccess(boost::json::value value)
{
  last_updates_running = false;
  const auto layer_id = std::exchange(last_updates_layer_id, std::string{});
  api.OnLastUpdates(layer_id, std::move(value));
}

void
SkySightRequest::OnLastUpdatesError(std::exception_ptr error) noexcept
{
  last_updates_running = false;
  last_updates_layer_id.clear();
  HandleAuthenticatedJsonError(std::move(error),
                               "SkySight last-updated request failed");
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
    if (HandleJsonRequestHttpStatus(http_error.status, http_error.retry_at,
                                    "SkySight datafiles request failed"))
      return;
  } catch (...) {
    if (!IsHostResolutionFailure(error)) {
      LogError(error, "SkySight datafiles request failed");
      LogFmt("SkySight forecast-step request will retry in {} seconds",
             DATAFILES_RETRY_SECONDS);
    }
    datafiles_retry_at = std::time(nullptr) + DATAFILES_RETRY_SECONDS;
    api.OnDatafilesRetry(layer_id);
    return;
  }

  api.OnDatafilesError(layer_id);
}

bool
SkySightRequest::HandleJsonRequestHttpStatus(unsigned status, time_t retry_at,
                                             const char *context) noexcept
{
  if (status == 401 || status == 403) {
    api_key.clear();
    valid_until = 0;
  }

  if (status == 429) {
    const auto now = std::time(nullptr);
    SetThrottleUntil(throttle_fallback.OnThrottle(now, retry_at));
    api.OnThrottle();
    LogThrottleNotice(retry_at > now);
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
    download_failures.OnSuccess(key);
    if (i->second->kind == FileJob::Kind::Generic)
      generic_keys.erase(key);

    switch (i->second->kind) {
    case FileJob::Kind::Generic:
      api.OnTileDownloadStateChanged();
      break;

    case FileJob::Kind::ForecastData:
      api.OnDatafileDownloaded(i->second->layer_id,
                               i->second->forecast_time,
                               SkySightFileDecoder::MakeDeferredPreparation(
                                 i->second->path));
      break;
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
  } catch (const SkySight::ResourceLimitError &) {
    if (kind == FileJob::Kind::Generic)
      download_failures.OnHttpFailure(key, 400, std::time(nullptr));
    else
      payload_retry_at[key] = std::numeric_limits<time_t>::max();

    LogError(error, kind == FileJob::Kind::ForecastData
             ? "SkySight forecast download exceeded its size limit"
             : "SkySight tile download exceeded its size limit");
  } catch (const HttpStatusError &http_error) {
    const auto now = std::time(nullptr);
    const auto retry_at = http_error.status == 429
      ? throttle_fallback.OnThrottle(now, http_error.retry_at)
      : http_error.retry_at;
    const auto decision = download_failures.OnHttpFailure(
      key, http_error.status, now, retry_at);
    if (http_error.status == 429) {
      if (kind == FileJob::Kind::Generic)
        live_tile_pacer.OnThrottle();
      if (kind == FileJob::Kind::Generic && failed_job != nullptr) {
        /* Retry the rejected tile as the sole recovery probe.  The renderer
           will repopulate the current viewport after that probe succeeds. */
        std::erase_if(pending_jobs, [](const auto &pending) {
          return pending.kind == FileJob::Kind::Generic;
        });
      }
      SetThrottleUntil(decision.ready_at);
      api.OnThrottle();
      LogThrottleNotice(http_error.retry_at > now);
    } else if (decision.action == SkySight::RequestFailureAction::Reauthenticate) {
      api_key.clear();
      valid_until = 0;
      last_login_request = 0;
    }

    if (decision.action != SkySight::RequestFailureAction::Terminal &&
        failed_job != nullptr) {
      terminal_forecast_error =
        !RequeueFileJob(*failed_job, decision.ready_at);
    } else {
      LogDownloadHttpError(kind == FileJob::Kind::ForecastData,
                           layer_id, forecast_time,
                           http_error.status, key);
    }
  } catch (...) {
    const bool host_resolution_failure = IsHostResolutionFailure(error);
    if (host_resolution_failure)
      download_failures.Erase(key);
    const auto decision = download_failures.OnTransportFailure(
      key, std::time(nullptr));
    if (failed_job != nullptr) {
      terminal_forecast_error =
        !RequeueFileJob(*failed_job, decision.ready_at);
    }
    if (!layer_id.empty() && !host_resolution_failure) {
      LogFmt("SkySight {} download failed for layer '{}' (forecast_time={})",
             kind == FileJob::Kind::ForecastData ? "forecast" : "tile",
             layer_id, (long long)forecast_time);
    }
    if (!host_resolution_failure)
      LogError(error,
               kind == FileJob::Kind::ForecastData
               ? "SkySight forecast download failed"
               : "SkySight tile download failed");
  }

  if (terminal_forecast_error)
    api.OnDatafileError(layer_id, forecast_time);

  TryPumpQueue();
  if (kind == FileJob::Kind::Generic)
    api.OnTileDownloadStateChanged();
}

AllocatedPath
SkySightRequest::GetThrottleCachePath() const noexcept
{
  return AllocatedPath::Build(cache_path,
                              SkySightCache::THROTTLE_CACHE_FILENAME);
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
SkySightRequest::LogThrottleNotice(bool server_retry_after) noexcept
{
  const auto now = std::time(nullptr);
  if (last_throttle_notice != 0 &&
      now < last_throttle_notice + SkySight::THROTTLE_FALLBACK_SECONDS)
    return;

  last_throttle_notice = now;
  LogFmt("SkySight throttled by server (HTTP 429), pausing requests for {} "
         "seconds ({})", unsigned(GetThrottleRemainingSeconds()),
         server_retry_after ? "server Retry-After" : "client fallback");
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
