// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The XCSoar Project

#include "SkySightRequest.hpp"
#include "SkysightAPI.hpp"
#include "Version.hpp"
#include "json/ParserOutputStream.hxx"
#include "co/Task.hxx"
#include "lib/curl/CoStreamRequest.hxx"
#include "lib/curl/Easy.hxx"
#include "lib/curl/Setup.hxx"
#include "lib/curl/Slist.hxx"
#include "io/ZipArchive.hpp"
#include "io/ZipReader.hpp"
#include "io/FileOutputStream.hxx"
#include "lib/fmt/RuntimeError.hxx"
#include "lib/curl/Global.hxx"
#include "LogFile.hpp"
#include "system/FileUtil.hpp"

#include <boost/json.hpp>

#include <algorithm>
#include <array>
#include <cstdlib>
#include <ctime>
#include <utility>

class HttpStatusError final : public std::runtime_error {
public:
  const unsigned status;

  explicit HttpStatusError(unsigned _status)
    :std::runtime_error("SkySight HTTP request failed"),
     status(_status) {}
};

static bool
HasForecastDataSuffix(std::string_view path) noexcept
{
  return path.ends_with(".nc") ||
    path.ends_with(".tif") || path.ends_with(".tiff") ||
    path.ends_with(".png") ||
    path.ends_with(".jpg") || path.ends_with(".jpeg");
}

static AllocatedPath
ExtractForecastArchive(Path archive_path)
{
  if (!archive_path.EndsWithIgnoreCase(".zip"))
    return archive_path;

  ZipArchive archive(archive_path);

  std::string entry_name;
  while (true) {
    entry_name = archive.NextName();
    if (entry_name.empty())
      throw std::runtime_error("SkySight forecast archive is empty");

    if (entry_name.back() != '/' && HasForecastDataSuffix(entry_name))
      break;
  }

  const auto suffix = Path{entry_name.c_str()}.GetSuffix();
  if (suffix == nullptr)
    throw std::runtime_error("SkySight forecast archive entry has no suffix");

  const auto output_path = archive_path.WithSuffix(suffix);
  if (File::Exists(output_path))
    return AllocatedPath(output_path.c_str());

  ZipReader reader(archive.get(), entry_name.c_str());
  FileOutputStream output(output_path);
  std::array<std::byte, 64 * 1024> buffer;

  while (true) {
    const auto nbytes = reader.Read(buffer);
    if (nbytes == 0)
      break;

    output.Write(std::span<const std::byte>{buffer.data(), nbytes});
  }

  output.Commit();
  return AllocatedPath(output_path.c_str());
}

static std::string
EscapeJsonString(std::string_view value)
{
  std::string escaped;
  escaped.reserve(value.size() + 8);

  for (const char ch : value) {
    switch (ch) {
    case '\\':
      escaped += "\\\\";
      break;
    case '"':
      escaped += "\\\"";
      break;
    case '\n':
      escaped += "\\n";
      break;
    case '\r':
      escaped += "\\r";
      break;
    case '\t':
      escaped += "\\t";
      break;
    default:
      escaped.push_back(ch);
      break;
    }
  }

  return escaped;
}

static Co::Task<boost::json::value>
LoginTask(CurlGlobal &curl, std::string email, std::string password)
{
  CurlEasy easy{"https://skysight.io/api/auth"};
  Curl::Setup(easy);

  CurlSlist headers;
  headers.Append("X-API-Key: XCSoar");
  headers.Append((std::string{"User-Agent: "} + XCSoar_ProductToken).c_str());
  headers.Append("Content-Type: application/json");

  easy.SetPost();
  easy.SetRequestHeaders(headers.Get());

  const auto json = std::string{"{\"username\":\""} +
                    EscapeJsonString(email) +
                    "\",\"password\":\"" +
                    EscapeJsonString(password) +
                    "\"}";
  easy.SetRequestBody(json);
  easy.SetFailOnError(false);

  Json::ParserOutputStream parser;
  const auto response = co_await Curl::CoStreamRequest(curl, std::move(easy), parser);
  if (response.status != 200 && response.status != 201)
    throw FmtRuntimeError("SkySight login failed with status {}", response.status);

  co_return parser.Finish();
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

  Json::ParserOutputStream parser;
  const auto response = co_await Curl::CoStreamRequest(curl, std::move(easy), parser);
  if (response.status != 200 && response.status != 201)
    throw HttpStatusError(response.status);

  co_return parser.Finish();
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

SkySightRequest::SkySightRequest(SkysightAPI &_api, CurlGlobal &_curl) noexcept
  :api(_api),
   curl(_curl),
   login_job(curl.GetEventLoop()),
   regions_job(curl.GetEventLoop()),
   layers_job(curl.GetEventLoop()),
   last_updates_job(curl.GetEventLoop()),
   datafiles_job(curl.GetEventLoop())
{
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

  for (auto &i : file_jobs)
    i.second->function.Cancel();

  file_jobs.clear();
  pending_jobs.clear();
  retry_after.clear();
  throttle_until = 0;
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

void
SkySightRequest::PumpQueue()
{
  CleanupFinishedJobs();

  const auto now = std::time(nullptr);
  if (now < throttle_until)
    return;

  while (file_jobs.size() < MAX_ACTIVE_DOWNLOADS && !pending_jobs.empty()) {
    auto job = std::move(pending_jobs.front());
    pending_jobs.pop_front();

    if (auto retry = retry_after.find(job.key); retry != retry_after.end()) {
      if (now < retry->second)
        continue;

      retry_after.erase(retry);
    }

    if (job.requires_auth && !IsLoggedIn()) {
      pending_jobs.push_front(std::move(job));
      EnsureLoggedIn();
      break;
    }

    auto active_job = std::make_unique<FileJob>(curl.GetEventLoop());
    auto *job_ptr = active_job.get();
    const auto key = job.key;
    job_ptr->kind = job.kind;
    job_ptr->path = std::move(job.path);
    job_ptr->layer_id = job.layer_id;
    job_ptr->forecast_time = job.forecast_time;

    file_jobs.emplace(key, std::move(active_job));
    job_ptr->function.Start(
      DownloadFileTask(curl, std::move(job.url),
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

void
SkySightRequest::DownloadDatafile(std::string_view layer_id,
                                  time_t forecast_time,
                                  std::string_view url,
                                  Path filename)
{
  PumpQueue();

  const std::string key{filename.c_str()};
  if (file_jobs.find(key) != file_jobs.end() || IsQueued(key))
    return;

  if (File::Exists(filename)) {
    try {
      api.OnDatafileDownloaded(layer_id, forecast_time,
                               ExtractForecastArchive(filename));
    } catch (...) {
      LogError(std::current_exception(), "SkySight forecast archive extraction failed");
      api.OnDatafileError(layer_id, forecast_time);
    }

    return;
  }

  pending_jobs.emplace_back(FileJob::Kind::ForecastData,
                            key, std::string{url},
                            AllocatedPath(filename.c_str()), true,
                            std::string{layer_id}, forecast_time);
  PumpQueue();
}

void
SkySightRequest::RequestRegions()
{
  if (regions_running)
    return;

  if (!HasCredentials())
    return;

  if (!IsLoggedIn()) {
    EnsureLoggedIn();
    return;
  }

  regions_running = true;
  regions_job.Start(
    JsonTask(curl, "https://skysight.io/api/regions", api_key),
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

  try {
    std::rethrow_exception(error);
  } catch (const HttpStatusError &http_error) {
    if (http_error.status == 401 || http_error.status == 403) {
      api_key.clear();
      valid_until = 0;
    }

    LogFmt("SkySight regions request failed with HTTP %u", http_error.status);
  } catch (...) {
    LogError(error, "SkySight regions request failed");
  }
}

void
SkySightRequest::RequestLayers(std::string_view region_id)
{
  if (region_id.empty() || layers_running)
    return;

  if (!HasCredentials())
    return;

  if (!IsLoggedIn()) {
    EnsureLoggedIn();
    return;
  }

  layers_running = true;

  std::string url{"https://skysight.io/api/layers?region_id="};
  url += region_id;

  layers_job.Start(
    JsonTask(curl, std::move(url), api_key),
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

  try {
    std::rethrow_exception(error);
  } catch (const HttpStatusError &http_error) {
    if (http_error.status == 401 || http_error.status == 403) {
      api_key.clear();
      valid_until = 0;
    }

    LogFmt("SkySight layers request failed with HTTP %u", http_error.status);
  } catch (...) {
    LogError(error, "SkySight layers request failed");
  }
}

void
SkySightRequest::RequestLastUpdates(std::string_view region_id)
{
  if (region_id.empty() || last_updates_running)
    return;

  if (!HasCredentials())
    return;

  if (!IsLoggedIn()) {
    EnsureLoggedIn();
    return;
  }

  last_updates_running = true;

  std::string url{"https://skysight.io/api/data/last_updated?region_id="};
  url += region_id;

  last_updates_job.Start(
    JsonTask(curl, std::move(url), api_key),
    [this](boost::json::value value) {
      OnLastUpdatesSuccess(std::move(value));
    },
    [this](std::exception_ptr error) {
      OnLastUpdatesError(std::move(error));
    });
}

void
SkySightRequest::RequestDatafiles(std::string_view region_id,
                                  std::string_view layer_id,
                                  time_t from_time)
{
  if (region_id.empty() || layer_id.empty() || datafiles_running)
    return;

  if (!HasCredentials())
    return;

  if (!IsLoggedIn()) {
    EnsureLoggedIn();
    return;
  }

  datafiles_running = true;
  datafiles_layer_id = std::string{layer_id};

  std::string url{"https://skysight.io/api/data?region_id="};
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
    if (http_error.status == 401 || http_error.status == 403) {
      api_key.clear();
      valid_until = 0;
    }

    LogFmt("SkySight last-updated request failed with HTTP %u", http_error.status);
  } catch (...) {
    LogError(error, "SkySight last-updated request failed");
  }
}

void
SkySightRequest::OnDatafilesSuccess(boost::json::value value)
{
  datafiles_running = false;

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
    if (http_error.status == 401 || http_error.status == 403) {
      api_key.clear();
      valid_until = 0;
    }

    LogFmt("SkySight datafiles request failed with HTTP %u", http_error.status);
  } catch (...) {
    LogError(error, "SkySight datafiles request failed");
  }

  api.OnDatafilesError(layer_id);
}

void
SkySightRequest::OnFileSuccess(const std::string &key) noexcept
{
  if (auto i = file_jobs.find(key); i != file_jobs.end()) {
    i->second->finished = true;

    try {
      switch (i->second->kind) {
      case FileJob::Kind::Generic:
        api.OnDownloadComplete();
        break;

      case FileJob::Kind::ForecastData:
        api.OnDatafileDownloaded(i->second->layer_id,
                                 i->second->forecast_time,
                                 ExtractForecastArchive(i->second->path));
        break;
      }
    } catch (...) {
      LogError(std::current_exception(), "SkySight forecast archive extraction failed");
      api.OnDatafileError(i->second->layer_id, i->second->forecast_time);
    }
  }
  PumpQueue();
}

void
SkySightRequest::OnFileError(const std::string &key,
                             std::exception_ptr error) noexcept
{
  if (auto i = file_jobs.find(key); i != file_jobs.end())
    i->second->finished = true;

  try {
    std::rethrow_exception(error);
  } catch (const HttpStatusError &http_error) {
    const auto retry_time = std::time(nullptr) +
      (http_error.status == 429 ? THROTTLE_RETRY_SECONDS : ERROR_RETRY_SECONDS);
    retry_after[key] = retry_time;

    if (http_error.status == 429) {
      throttle_until = retry_time;
      LogFmt("SkySight throttled by server (HTTP 429), pausing tile downloads for %u seconds",
             unsigned(THROTTLE_RETRY_SECONDS));
    } else {
      LogFmt("SkySight tile download failed with HTTP %u", http_error.status);
    }
  } catch (...) {
    retry_after[key] = std::time(nullptr) + ERROR_RETRY_SECONDS;
    LogError(error, "SkySight tile download failed");
  }

  PumpQueue();
}
