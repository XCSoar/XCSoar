// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The XCSoar Project

#include "Http.hpp"
#include "net/client/auth/JwtBearerSession.hpp"
#include "net/http/Progress.hpp"
#include "io/OutputStream.hxx"
#include "io/StringOutputStream.hxx"
#include "lib/curl/CoStreamRequest.hxx"
#include "lib/curl/Easy.hxx"
#include "lib/curl/Setup.hxx"
#include "lib/curl/Slist.hxx"
#include "co/InjectTask.hxx"
#include "co/Task.hxx"
#include "lib/curl/Global.hxx"
#include "util/BindMethod.hxx"

#include <condition_variable>
#include <exception>
#include <mutex>

namespace XCTherm::Http {

namespace {

class CancelCheckOutputStream final : public OutputStream {
  OutputStream &destination;
  const std::function<bool()> &should_continue;

public:
  CancelCheckOutputStream(OutputStream &_destination,
                          const std::function<bool()> &_should_continue) noexcept
    :destination(_destination),
     should_continue(_should_continue) {}

  void Write(std::span<const std::byte> data) override {
    if (!should_continue())
      throw TransferCancelled{};
    destination.Write(data);
  }
};

Co::Task<Curl::CoResponse>
CoGetWithHeaders(CurlGlobal &curl, const char *url, curl_slist *headers,
                 ProgressListener *progress,
                 const std::function<bool()> &should_continue)
{
  CurlEasy easy{url};
  Curl::Setup(easy);
  easy.SetRequestHeaders(headers);
  easy.SetOption(CURLOPT_ACCEPT_ENCODING, "gzip");
  easy.SetOption(CURLOPT_FOLLOWLOCATION, 1L);
  easy.SetTimeout(60);

  StringOutputStream body;
  CancelCheckOutputStream checked{body, should_continue};

  std::optional<Net::ProgressAdapter> progress_adapter;
  if (progress != nullptr)
    progress_adapter.emplace(easy, *progress);

  Curl::CoResponse response =
    co_await Curl::CoStreamRequest(curl, std::move(easy), checked);

  /* CoStreamRequest writes to @p body, not CoResponse::body. */
  response.body = std::move(body).GetValue();
  co_return response;
}

} // anonymous namespace

Co::Task<Curl::CoResponse>
CoGet(CurlGlobal &curl, const char *url, ProgressListener *progress,
      const std::function<bool()> &should_continue)
{
  co_return co_await CoGetWithHeaders(curl, url, nullptr, progress,
                                      should_continue);
}

Co::Task<Curl::CoResponse>
CoBearerGet(CurlGlobal &curl, const char *url,
            Net::Client::Auth::JwtBearerSession &session,
            ProgressListener *progress,
            const std::function<bool()> &should_continue,
            const bool retry_on_401)
{
  auto do_get = [&]() -> Co::Task<Curl::CoResponse> {
    CurlSlist headers;
    headers.Append(session.GetAuthHeader().c_str());
    co_return co_await CoGetWithHeaders(curl, url, headers.Get(), progress,
                                        should_continue);
  };

  auto response = co_await do_get();

  if (response.status == 401 && retry_on_401 &&
      session.ForceReauthenticate())
    response = co_await do_get();

  co_return response;
}

namespace {

struct RunSyncCompletion {
  std::mutex mutex;
  std::condition_variable cv;
  std::exception_ptr error;
  bool done = false;

  void OnDone(std::exception_ptr e) noexcept {
    const std::lock_guard lock{mutex};
    error = std::move(e);
    done = true;
    cv.notify_one();
  }
};

} // namespace

void
RunSync(CurlGlobal &curl, Co::InvokeTask task)
{
  /* Blocking helper: caller must not be the curl/asio event-loop thread,
     otherwise completion cannot be delivered and this wait deadlocks. */
  RunSyncCompletion completion;

  Co::InjectTask inject{curl.GetEventLoop()};
  inject.Start(std::move(task),
                BIND_METHOD(completion, &RunSyncCompletion::OnDone));

  std::unique_lock lock{completion.mutex};
  completion.cv.wait(lock, [&]() { return completion.done; });

  if (completion.error)
    std::rethrow_exception(std::move(completion.error));
}

} // namespace XCTherm::Http
