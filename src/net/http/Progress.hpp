// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The XCSoar Project

#pragma once

#include <curl/system.h>

class CurlEasy;
class ProgressListener;

namespace Net {

/**
 * This class reports upload and download progress to an
 * #OperationEnvironment instance.
 */
class ProgressAdapter {
  ProgressListener &listener;

public:
  /**
   * Register as "xferinfo" callback with the given #CurlEasy and keep
   * reporting progress to the #OperationEnvironment instance.
   */
  ProgressAdapter(CurlEasy &curl, ProgressListener &listener);

private:
  void Callback(curl_off_t dltotal, curl_off_t dlnow,
                curl_off_t ultotal, curl_off_t ulnow) noexcept;

  static int _Callback(void *clientp, curl_off_t dltotal, curl_off_t dlnow,
                       curl_off_t ultotal, curl_off_t ulnow) noexcept;
};

} // namespace Net
