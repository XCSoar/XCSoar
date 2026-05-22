// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The XCSoar Project

#include "Http.hpp"

#include "LogFile.hpp"
#include "io/StringOutputStream.hxx"
#include "lib/curl/Easy.hxx"
#include "lib/curl/Setup.hxx"

#include <curl/curl.h>
#include <span>

namespace XCThermHttp {

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
  const ProgressFn *progress = nullptr;
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
  std::string refresh_cookie_line;
};

size_t
HeaderCallback(char *buffer, size_t size, size_t nitems,
               void *userdata) noexcept
{
  auto *capture = static_cast<HeaderCapture *>(userdata);
  const size_t total = size * nitems;
  if (capture == nullptr)
    return total;

  std::string line(buffer, total);
  if (line.size() > 12 &&
      (line.substr(0, 11) == "Set-Cookie:" ||
       line.substr(0, 11) == "set-cookie:") &&
      line.find("refreshToken") != std::string::npos)
    capture->refresh_cookie_line = line.substr(12);

  return total;
}

bool
Perform(CurlEasy &easy, StringOutputStream &body, Response &response,
        HeaderCapture *headers, ProgressFn progress) noexcept
{
  response = {};

  easy.SetWriteFunction(WriteToStream, &body);
  if (headers != nullptr) {
    easy.SetHeaderFunction(HeaderCallback, headers);
  }

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
    LogFmt("xctherm http: transfer failed");
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
    response.refresh_cookie_line = std::move(headers->refresh_cookie_line);

  return true;
}

} // anonymous namespace

bool
PerformGet(const char *url, curl_slist *headers, bool accept_gzip,
           Response &response, ProgressFn progress) noexcept
{
  try {
    CurlEasy easy{url};
    Curl::Setup(easy);
    easy.SetRequestHeaders(headers);
    easy.SetOption(CURLOPT_FOLLOWLOCATION, 1L);
    easy.SetTimeout(60);

    if (accept_gzip)
      easy.SetOption(CURLOPT_ACCEPT_ENCODING, "gzip");

    StringOutputStream body;
    return Perform(easy, body, response, nullptr, std::move(progress));
  } catch (...) {
    return false;
  }
}

bool
PerformPost(const char *url, curl_slist *headers, std::string_view body,
            Response &response, bool capture_cookies,
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

    StringOutputStream response_body;
    HeaderCapture header_capture;
    HeaderCapture *capture = capture_cookies ? &header_capture : nullptr;
    return Perform(easy, response_body, response, capture, nullptr);
  } catch (...) {
    return false;
  }
}

} // namespace XCThermHttp
