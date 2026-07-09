// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The XCSoar Project

#pragma once

#include <cstdint>
#include <functional>
#include <string>
#include <string_view>

struct curl_slist;

class ProgressListener;

namespace Net::Client {

struct SyncHttpResponse {
  std::string body;
  long http_code = 0;
  int64_t size_download = 0;
  int64_t speed_download = 0;

  /** Value part of a captured Set-Cookie line (without header prefix). */
  std::string captured_set_cookie;
};

using SyncHttpProgressFn = std::function<bool(uint64_t bytes_now,
                                              uint64_t bytes_total)>;

/**
 * Synchronous HTTP GET/POST via CurlEasy (blocking; not for the asio thread).
 */
bool
SyncHttpGet(const char *url, curl_slist *headers, bool accept_gzip,
            SyncHttpResponse &response,
            const char *capture_set_cookie_name = nullptr,
            SyncHttpProgressFn progress = nullptr) noexcept;

bool
SyncHttpPost(const char *url, curl_slist *headers, std::string_view body,
             SyncHttpResponse &response,
             const char *capture_set_cookie_name = nullptr,
             const char *cookie = nullptr) noexcept;

/**
 * Bridge #ProgressListener to #SyncHttpProgressFn. @p should_continue
 * is polled on each progress tick; returning false aborts the transfer.
 */
[[nodiscard]]
SyncHttpProgressFn
MakeSyncHttpProgress(ProgressListener *listener,
                     const std::function<bool()> &should_continue) noexcept;

} // namespace Net::Client
