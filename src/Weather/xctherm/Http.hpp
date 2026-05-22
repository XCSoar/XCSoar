// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The XCSoar Project

#pragma once

#include <cstdint>
#include <functional>
#include <string>
#include <string_view>

struct curl_slist;

namespace XCThermHttp {

struct Response {
  std::string body;
  long http_code = 0;
  int64_t size_download = 0;
  int64_t speed_download = 0;
  std::string refresh_cookie_line;
};

using ProgressFn = std::function<bool(uint64_t bytes_now,
                                    uint64_t bytes_total)>;

/**
 * Perform an HTTP GET with optional gzip decompression and progress.
 */
bool
PerformGet(const char *url, curl_slist *headers, bool accept_gzip,
           Response &response, ProgressFn progress = nullptr) noexcept;

/**
 * Perform an HTTP POST. Optionally capture Set-Cookie refreshToken line.
 */
bool
PerformPost(const char *url, curl_slist *headers, std::string_view body,
            Response &response, bool capture_cookies = false,
            const char *cookie = nullptr) noexcept;

} // namespace XCThermHttp
