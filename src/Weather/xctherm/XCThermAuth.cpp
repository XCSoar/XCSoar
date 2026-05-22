// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The XCSoar Project

#include "XCThermAuth.hpp"
#include "LogFile.hpp"

#include <cstring>
#include <ctime>
#include <string_view>
#include <vector>

#include <curl/curl.h>

/* ------------------------------------------------------------------ */
/* CURL helpers                                                        */
/* ------------------------------------------------------------------ */

static std::string
EscapeJsonString(std::string_view s)
{
  std::string out;
  out.reserve(s.size() + 8);
  for (const char c : s) {
    switch (c) {
    case '\\':
      out += "\\\\";
      break;
    case '"':
      out += "\\\"";
      break;
    case '\n':
      out += "\\n";
      break;
    case '\r':
      out += "\\r";
      break;
    case '\t':
      out += "\\t";
      break;
    default:
      if ((unsigned char)c >= 0x20)
        out += c;
      break;
    }
  }
  return out;
}

static size_t
WriteCallback(void *contents, size_t size, size_t nmemb,
              std::vector<uint8_t> *buffer) {
  const size_t total = size * nmemb;
  auto *ptr = static_cast<uint8_t *>(contents);
  buffer->insert(buffer->end(), ptr, ptr + total);
  return total;
}

/**
 * Header callback to capture Set-Cookie containing refreshToken.
 */
struct HeaderCapture {
  std::string cookie_header;
};

static size_t
HeaderCallback(char *buffer, size_t size, size_t nitems,
               HeaderCapture *capture) {
  const size_t total = size * nitems;
  std::string line(buffer, total);

  /* Look for Set-Cookie header containing refreshToken */
  if (line.size() > 12 &&
      (line.substr(0, 11) == "Set-Cookie:" ||
       line.substr(0, 11) == "set-cookie:")) {
    if (line.find("refreshToken") != std::string::npos)
      capture->cookie_header = line.substr(12);
  }

  return total;
}

/* ------------------------------------------------------------------ */
/* XCThermAuth                                                         */
/* ------------------------------------------------------------------ */

void
XCThermAuth::SetCredentials(const std::string &email,
                            const std::string &password) noexcept
{
  if (credentials.email != email || credentials.password != password) {
    credentials.email = email;
    credentials.password = password;
    /* Invalidate tokens on credential change */
    jwt_token.clear();
    refresh_token.clear();
    jwt_expiry = 0;
  }
}

bool
XCThermAuth::HasValidToken() const noexcept
{
  return !jwt_token.empty() &&
         jwt_expiry > static_cast<uint32_t>(std::time(nullptr));
}

std::string
XCThermAuth::GetAuthHeader() const noexcept
{
  return "Authorization: Bearer " + jwt_token;
}

bool
XCThermAuth::EnsureValidToken() noexcept
{
  if (HasValidToken())
    return true;

  /* Try refresh first if we have a refresh token */
  if (!refresh_token.empty()) {
    if (RefreshJWT())
      return true;
    LogFmt("xctherm: refresh failed, falling back to full auth");
  }

  return Authenticate();
}

bool
XCThermAuth::ForceReauthenticate() noexcept
{
  jwt_token.clear();
  jwt_expiry = 0;
  return Authenticate();
}

/* ------------------------------------------------------------------ */
/* Full authentication                                                 */
/* ------------------------------------------------------------------ */

bool
XCThermAuth::Authenticate() noexcept
{
  if (credentials.email.empty() || credentials.password.empty()) {
    LogFmt("xctherm: auth skipped — no credentials");
    return false;
  }

  LogFmt("xctherm: authenticating");

  const std::string post_data =
    R"({"email":")" + EscapeJsonString(credentials.email) +
    R"(","password":")" + EscapeJsonString(credentials.password) + R"("})";

  CURL *curl = curl_easy_init();
  if (!curl)
    return false;

  std::vector<uint8_t> response_buffer;
  HeaderCapture header_capture;

  struct curl_slist *headers = nullptr;
  headers = curl_slist_append(headers, "Content-Type: application/json");

  curl_easy_setopt(curl, CURLOPT_URL,
                   "https://xctherm.com/api/accounts/authenticate");
  curl_easy_setopt(curl, CURLOPT_POSTFIELDS, post_data.c_str());
  curl_easy_setopt(curl, CURLOPT_HTTPHEADER, headers);
  curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, WriteCallback);
  curl_easy_setopt(curl, CURLOPT_WRITEDATA, &response_buffer);
  curl_easy_setopt(curl, CURLOPT_HEADERFUNCTION, HeaderCallback);
  curl_easy_setopt(curl, CURLOPT_HEADERDATA, &header_capture);
  curl_easy_setopt(curl, CURLOPT_CONNECTTIMEOUT, 15L);
  curl_easy_setopt(curl, CURLOPT_TIMEOUT, 30L);
  curl_easy_setopt(curl, CURLOPT_NOSIGNAL, 1L);

  CURLcode res = curl_easy_perform(curl);
  long http_code = 0;
  curl_easy_getinfo(curl, CURLINFO_RESPONSE_CODE, &http_code);

  curl_slist_free_all(headers);
  curl_easy_cleanup(curl);

  if (res != CURLE_OK || http_code != 200) {
    LogFmt("xctherm: auth failed curl={} http={}", (int)res, http_code);
    return false;
  }

  std::string response(response_buffer.begin(), response_buffer.end());

  /* Parse JWT */
  if (!ParseJWTFromResponse(response, jwt_token)) {
    LogFmt("xctherm: no jwtToken in auth response");
    return false;
  }

  jwt_expiry = static_cast<uint32_t>(std::time(nullptr)) + JWT_LIFETIME_SECONDS;

  /* Parse refresh token from cookie */
  if (!header_capture.cookie_header.empty()) {
    ParseRefreshTokenFromCookie(header_capture.cookie_header, refresh_token);
    if (!refresh_token.empty())
      LogFmt("xctherm: got refreshToken (len={})", refresh_token.size());
  }

  LogFmt("xctherm: auth success, jwt_len={}", jwt_token.size());
  return true;
}

/* ------------------------------------------------------------------ */
/* Token refresh                                                       */
/* ------------------------------------------------------------------ */

bool
XCThermAuth::RefreshJWT() noexcept
{
  if (refresh_token.empty())
    return false;

  LogFmt("xctherm: refreshing JWT");

  CURL *curl = curl_easy_init();
  if (!curl)
    return false;

  std::vector<uint8_t> response_buffer;
  HeaderCapture header_capture;

  /* Send refresh token as cookie */
  const std::string cookie = "refreshToken=" + refresh_token;

  struct curl_slist *headers = nullptr;
  headers = curl_slist_append(headers, "Content-Type: application/json");

  curl_easy_setopt(curl, CURLOPT_URL,
                   "https://xctherm.com/api/accounts/refresh-token");
  curl_easy_setopt(curl, CURLOPT_POST, 1L);
  curl_easy_setopt(curl, CURLOPT_POSTFIELDS, "");
  curl_easy_setopt(curl, CURLOPT_COOKIE, cookie.c_str());
  curl_easy_setopt(curl, CURLOPT_HTTPHEADER, headers);
  curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, WriteCallback);
  curl_easy_setopt(curl, CURLOPT_WRITEDATA, &response_buffer);
  curl_easy_setopt(curl, CURLOPT_HEADERFUNCTION, HeaderCallback);
  curl_easy_setopt(curl, CURLOPT_HEADERDATA, &header_capture);
  curl_easy_setopt(curl, CURLOPT_CONNECTTIMEOUT, 10L);
  curl_easy_setopt(curl, CURLOPT_TIMEOUT, 20L);
  curl_easy_setopt(curl, CURLOPT_NOSIGNAL, 1L);

  CURLcode res = curl_easy_perform(curl);
  long http_code = 0;
  curl_easy_getinfo(curl, CURLINFO_RESPONSE_CODE, &http_code);

  curl_slist_free_all(headers);
  curl_easy_cleanup(curl);

  if (res != CURLE_OK || http_code != 200) {
    LogFmt("xctherm: refresh failed curl={} http={}", (int)res, http_code);
    refresh_token.clear();
    return false;
  }

  std::string response(response_buffer.begin(), response_buffer.end());

  if (!ParseJWTFromResponse(response, jwt_token)) {
    LogFmt("xctherm: no jwtToken in refresh response");
    return false;
  }

  jwt_expiry = static_cast<uint32_t>(std::time(nullptr)) + JWT_LIFETIME_SECONDS;

  /* Update refresh token if a new one was issued */
  if (!header_capture.cookie_header.empty()) {
    std::string new_refresh;
    if (ParseRefreshTokenFromCookie(header_capture.cookie_header, new_refresh))
      refresh_token = std::move(new_refresh);
  }

  LogFmt("xctherm: refresh success, jwt_len={}", jwt_token.size());
  return true;
}

/* ------------------------------------------------------------------ */
/* JSON/Cookie parsing (simple, no external JSON lib)                  */
/* ------------------------------------------------------------------ */

bool
XCThermAuth::ParseJWTFromResponse(const std::string &response,
                                  std::string &out_token) noexcept
{
  /* Find "jwtToken":"<value>" */
  const char *needle = "\"jwtToken\":\"";
  size_t pos = response.find(needle);
  if (pos == std::string::npos)
    return false;

  pos += std::strlen(needle);
  size_t end = response.find('"', pos);
  if (end == std::string::npos)
    return false;

  out_token = response.substr(pos, end - pos);
  return !out_token.empty();
}

bool
XCThermAuth::ParseRefreshTokenFromCookie(const std::string &cookie_header,
                                         std::string &out_token) noexcept
{
  /* Cookie header looks like: "refreshToken=<value>; Path=/; ..." */
  const char *needle = "refreshToken=";
  size_t pos = cookie_header.find(needle);
  if (pos == std::string::npos)
    return false;

  pos += std::strlen(needle);
  size_t end = cookie_header.find(';', pos);
  if (end == std::string::npos)
    end = cookie_header.size();

  /* Trim whitespace */
  while (end > pos && (cookie_header[end - 1] == ' ' ||
                       cookie_header[end - 1] == '\r' ||
                       cookie_header[end - 1] == '\n'))
    --end;

  out_token = cookie_header.substr(pos, end - pos);
  return !out_token.empty();
}
