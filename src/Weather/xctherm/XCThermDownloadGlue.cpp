// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The XCSoar Project

#include "XCThermDownloadGlue.hpp"
#include "XCThermDownloadJob.hpp"
#include "LogFile.hpp"
#include "lib/curl/Global.hxx"
#include "util/Macros.hpp"

#include <utility>

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
  completion_error = {};
}

void
XCThermDownloadGlue::Start(
  std::shared_ptr<XCThermDownloadJob> new_job,
  std::function<void(std::shared_ptr<XCThermDownloadJob>)> &&finished)
{
  if (task.IsShuttingDown() || task.IsRunning())
    return;

  job = std::move(new_job);
  on_finished = std::move(finished);
  completion_error = {};

  task.Start(RunDownload(), BIND_THIS_METHOD(OnCompletion));
}

void
XCThermDownloadGlue::RequestCancel() noexcept
{
  if (job != nullptr)
    job->cancel.store(true);
}

Co::InvokeTask
XCThermDownloadGlue::RunDownload()
{
  if (job != nullptr)
    co_await RunXCThermDownload(curl, job);
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

  auto finished_job = std::move(job);
  auto callback = std::move(on_finished);
  on_finished = nullptr;
  completion_error = {};

  if (finished_job != nullptr && callback)
    callback(std::move(finished_job));
}
