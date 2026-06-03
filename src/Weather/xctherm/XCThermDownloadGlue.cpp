// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The XCSoar Project

#include "XCThermDownloadGlue.hpp"
#include "XCThermAPI.hpp"
#include "XCThermDownloadJob.hpp"
#include "Components.hpp"
#include "NetComponents.hpp"
#include "LogFile.hpp"
#include "lib/curl/Global.hxx"
#include "util/Macros.hpp"

#include <utility>

XCThermDownloadGlue *
GetXCThermDownloadGlue() noexcept
{
  if (net_components == nullptr)
    return nullptr;
  return net_components->xctherm_download.get();
}

XCThermDownloadGlue::XCThermDownloadGlue(CurlGlobal &_curl) noexcept
  :curl(_curl),
   task(curl.GetEventLoop())
{
}

void
XCThermDownloadGlue::BeginShutdown() noexcept
{
  if (job != nullptr)
    job->cancel.store(true);

  task.BeginShutdown();
  complete_notify.ClearNotification();
  job.reset();
  on_finished = nullptr;
  index_fetch_callbacks.clear();
  index_fetch = false;
  completion_error = {};
}

void
XCThermDownloadGlue::Start(
  std::shared_ptr<XCThermDownloadJob> new_job,
  std::function<void(std::shared_ptr<XCThermDownloadJob>)> &&finished)
{
  if (task.IsShuttingDown() || task.IsRunning())
    return;

  index_fetch = false;
  index_fetch_callbacks.clear();
  job = std::move(new_job);
  on_finished = std::move(finished);
  completion_error = {};

  task.Start(RunDownload(), BIND_THIS_METHOD(OnCompletion));
}

void
XCThermDownloadGlue::StartIndexFetch(std::function<void()> &&finished)
{
  if (XCThermAPI::Instance().IsIndexLoaded()) {
    if (finished)
      finished();
    return;
  }

  if (finished)
    index_fetch_callbacks.push_back(std::move(finished));

  if (index_fetch || task.IsShuttingDown() || task.IsRunning())
    return;

  index_fetch = true;
  job.reset();
  on_finished = nullptr;
  completion_error = {};

  task.Start(RunIndexFetch(), BIND_THIS_METHOD(OnCompletion));
}

void
XCThermDownloadGlue::RequestCancel() noexcept
{
  if (job != nullptr)
    job->cancel.store(true);
}

void
XCThermDownloadGlue::Abandon() noexcept
{
  RequestCancel();
  on_finished = nullptr;
}

Co::InvokeTask
XCThermDownloadGlue::RunDownload()
{
  if (job != nullptr)
    co_await RunXCThermDownload(curl, job);
  co_return;
}

Co::InvokeTask
XCThermDownloadGlue::RunIndexFetch()
{
  try {
    co_await XCThermAPI::Instance().CoEnsureIndexLoaded(curl);
  } catch (const std::exception &e) {
    LogFmt("xctherm: background index fetch failed: {}", e.what());
  } catch (...) {
    LogFmt("xctherm: background index fetch failed (unknown)");
  }
  co_return;
}

void
XCThermDownloadGlue::OnCompletion(std::exception_ptr error) noexcept
{
  completion_error = std::move(error);
  if (completion_error)
    LogError(completion_error, "XCTherm download coroutine failed");

  complete_notify.SendNotification();
}

void
XCThermDownloadGlue::OnCompleteNotify() noexcept
{
  if (task.IsShuttingDown())
    return;

  if (completion_error && job != nullptr && !job->error_eptr)
    job->error_eptr = completion_error;

  if (index_fetch) {
    index_fetch = false;
    auto callbacks = std::move(index_fetch_callbacks);
    index_fetch_callbacks.clear();
    completion_error = {};

    for (auto &callback : callbacks) {
      if (callback)
        callback();
    }
    return;
  }

  auto finished_job = std::move(job);
  auto callback = std::move(on_finished);
  on_finished = nullptr;
  completion_error = {};

  if (finished_job != nullptr && callback)
    callback(std::move(finished_job));
}
