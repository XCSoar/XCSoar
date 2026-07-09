// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The XCSoar Project

#include "XCThermDownloadGlue.hpp"
#include "XCThermAPI.hpp"
#include "XCThermDownloadJob.hpp"
#include "Weather/BackgroundDownloadProgress.hpp"
#include "Components.hpp"
#include "Language/Language.hpp"
#include "NetComponents.hpp"
#include "LogFile.hpp"
#include "lib/curl/Global.hxx"
#include "util/Macros.hpp"
#include "util/StaticString.hxx"

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
  on_index_fetched = nullptr;
  index_fetch = false;
  completion_error = {};
  BackgroundDownloadProgress::Get().ForceHide();
}

void
XCThermDownloadGlue::Start(
  std::shared_ptr<XCThermDownloadJob> new_job,
  std::function<void(std::shared_ptr<XCThermDownloadJob>)> &&finished)
{
  if (task.IsShuttingDown() || task.IsRunning())
    return;

  index_fetch = false;
  on_index_fetched = nullptr;
  job = std::move(new_job);
  on_finished = std::move(finished);
  completion_error = {};

  StaticString<128> text;
  text.Format(_("Downloading XCTherm %s..."),
              gettext(job->target_label.c_str()));
  BackgroundDownloadProgress::Get().Begin(text.c_str());
  BackgroundDownloadProgress::Get().SetProgressRange(100);

  task.Start(RunDownload(), BIND_THIS_METHOD(OnCompletion));
}

void
XCThermDownloadGlue::StartIndexFetch(std::function<void()> &&finished)
{
  if (task.IsShuttingDown() || task.IsRunning())
    return;

  if (XCThermAPI::Instance().IsIndexLoaded()) {
    if (finished)
      finished();
    return;
  }

  index_fetch = true;
  on_index_fetched = std::move(finished);
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
    auto callback = std::move(on_index_fetched);
    on_index_fetched = nullptr;
    completion_error = {};

    if (callback)
      callback();
    return;
  }

  if (!task.IsRunning())
    BackgroundDownloadProgress::Get().End();

  auto finished_job = std::move(job);
  auto callback = std::move(on_finished);
  on_finished = nullptr;
  completion_error = {};

  if (finished_job != nullptr && callback)
    callback(std::move(finished_job));
}
