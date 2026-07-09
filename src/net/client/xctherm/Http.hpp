// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The XCSoar Project

#pragma once

#include "co/InvokeTask.hxx"
#include "co/Task.hxx"
#include "lib/curl/CoRequest.hxx"

#include <functional>

class CurlGlobal;
class ProgressListener;

namespace Net::Client::Auth {
class JwtBearerSession;
}

namespace XCTherm::Http {

/** Base URL for XCTherm forecast tiles and index. */
inline constexpr const char *kForecastBaseUrl =
  "https://tiles.xctherm.com/forecast";

/** Thrown from the write path when @p should_continue returns false. */
struct TransferCancelled {};

/**
 * GET with gzip, redirects, optional progress and cancel checks.
 */
Co::Task<Curl::CoResponse>
CoGet(CurlGlobal &curl, const char *url,
      ProgressListener *progress = nullptr,
      const std::function<bool()> &should_continue = []() { return true; });

/**
 * GET with @c Authorization: Bearer from @p session. On HTTP 401, calls
 * ForceReauthenticate() once and retries when @p retry_on_401 is true.
 */
Co::Task<Curl::CoResponse>
CoBearerGet(CurlGlobal &curl, const char *url,
            Net::Client::Auth::JwtBearerSession &session,
            ProgressListener *progress,
            const std::function<bool()> &should_continue,
            bool retry_on_401 = true);

/**
 * Run a coroutine to completion on @p curl's event loop (blocks caller).
 * For short UI-thread fetches only (e.g. index.json).
 */
void
RunSync(CurlGlobal &curl, Co::InvokeTask task);

} // namespace XCTherm::Http
