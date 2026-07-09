// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The XCSoar Project

#include "SyncHttp.hpp"

#include "Operation/ProgressListener.hpp"
#include "io/StringOutputStream.hxx"
#include "lib/curl/Easy.hxx"
#include "lib/curl/Setup.hxx"

#include <climits>
#include <curl/curl.h>
#include <span>

namespace Net::Client {

namespace {

size_t
WriteToStream(char *ptr, size_t size, size_t nmemb, void *userdata) noexcept
{
  auto *os = static_cast<StringOutputStream *>(userdata);
  const size_t total = size * nmemb;
  try {
    os->Write(std::span{
      reinterpret_cast<const std::byte *>(ptr), total});
    return total;
  } catch (...) {
    return 0;
  }
}

struct ProgressCtx {
  const SyncHttpProgressFn *progress = nullptr;
};

int
XferInfoCallback(void *userp,
                 curl_off_t dltotal, curl_off_t dlnow,
                 [[maybe_unused]] curl_off_t ultotal,
                 [[maybe_unused]] curl_off_t ulnow) noexcept
{
  auto *ctx = static_cast<ProgressCtx *>(userp);
  if (ctx == nullptr || ctx->progress == nullptr || !*ctx->progress)
    return 0;

  return (*ctx->progress)((uint64_t)dlnow, (uint64_t)dltotal) ? 0 : 1;
}

struct HeaderCapture {
  const char *cookie_name = nullptr;
  std::string set_cookie_value;
};

size_t
HeaderCallback(char *buffer, size_t size, size_t nitems,
               void *userdata) noexcept
{
  auto *capture = static_cast<HeaderCapture *>(userdata);
  const size_t total = size * nitems;
  if (capture == nullptr || capture->cookie_name == nullptr)
    return total;

  try {
    std::string line(buffer, total);
    if (line.size() <= 12)
      return total;

    if (line.substr(0, 11) != "Set-Cookie:" &&
        line.substr(0, 11) != "set-cookie:")
      return total;

    if (line.find(capture->cookie_name) == std::string::npos)
      return total;

    capture->set_cookie_value = line.substr(12);
  } catch (...) {
  }
  return total;
}

bool
Perform(CurlEasy &easy, StringOutputStream &body, SyncHttpResponse &response,
        HeaderCapture *headers, SyncHttpProgressFn progress)
{
  response = {};

  easy.SetWriteFunction(WriteToStream, &body);
  if (headers != nullptr)
    easy.SetHeaderFunction(HeaderCallback, headers);

  ProgressCtx progress_ctx;
  if (progress) {
    progress_ctx.progress = &progress;
    easy.SetXferInfoFunction(XferInfoCallback, &progress_ctx);
  } else {
    easy.SetNoProgress();
  }

  try {
    easy.Perform();
  } catch (...) {
    return false;
  }

  easy.GetInfo(CURLINFO_RESPONSE_CODE, &response.http_code);
  curl_off_t size_download = 0;
  curl_off_t speed_download = 0;
  easy.GetInfo(CURLINFO_SIZE_DOWNLOAD_T, &size_download);
  easy.GetInfo(CURLINFO_SPEED_DOWNLOAD_T, &speed_download);
  response.size_download = (int64_t)size_download;
  response.speed_download = (int64_t)speed_download;
  response.body = body.GetValue();

  if (headers != nullptr)
    response.captured_set_cookie =
      std::move(headers->set_cookie_value);

  return true;
}

} // anonymous namespace

bool
SyncHttpGet(const char *url, curl_slist *headers, bool accept_gzip,
            SyncHttpResponse &response, const char *capture_set_cookie_name,
            SyncHttpProgressFn progress) noexcept
{
  try {
    CurlEasy easy{url};
    Curl::Setup(easy);
    easy.SetRequestHeaders(headers);
    easy.SetOption(CURLOPT_FOLLOWLOCATION, 1L);
    easy.SetTimeout(60);

    if (accept_gzip)
      easy.SetOption(CURLOPT_ACCEPT_ENCODING, "gzip");

    HeaderCapture header_capture;
    HeaderCapture *capture = nullptr;
    if (capture_set_cookie_name != nullptr) {
      header_capture.cookie_name = capture_set_cookie_name;
      capture = &header_capture;
    }

    StringOutputStream body;
    if (!Perform(easy, body, response, capture, std::move(progress)))
      return false;

    return true;
  } catch (...) {
    return false;
  }
}

bool
SyncHttpPost(const char *url, curl_slist *headers, std::string_view body,
             SyncHttpResponse &response, const char *capture_set_cookie_name,
             const char *cookie) noexcept
{
  try {
    CurlEasy easy{url};
    Curl::Setup(easy);
    easy.SetRequestHeaders(headers);
    easy.SetPost();
    easy.SetRequestBody(body);
    easy.SetTimeout(30);

    if (cookie != nullptr)
      easy.SetOption(CURLOPT_COOKIE, cookie);

    HeaderCapture header_capture;
    HeaderCapture *capture = nullptr;
    if (capture_set_cookie_name != nullptr) {
      header_capture.cookie_name = capture_set_cookie_name;
      capture = &header_capture;
    }

    StringOutputStream response_body;
    return Perform(easy, response_body, response, capture, nullptr);
  } catch (...) {
    return false;
  }
}

SyncHttpProgressFn
MakeSyncHttpProgress(ProgressListener *listener,
                     const std::function<bool()> &should_continue) noexcept
{
  return [listener, should_continue](uint64_t bytes_now,
                                    uint64_t bytes_total) -> bool {
    if (!should_continue())
      return false;

    if (listener == nullptr)
      return true;

    if (bytes_total == 0)
      bytes_now = 0;

    if (bytes_total == 0 || bytes_total > UINT_MAX)
      return true;

    listener->SetProgressRange((unsigned)bytes_total);
    listener->SetProgressPosition((unsigned)bytes_now);
    return true;
  };
}

} // namespace Net::Client
