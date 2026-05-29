// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The XCSoar Project

#include "XCThermAuth.hpp"
#include "LogFile.hpp"
#include "lib/curl/Easy.hxx"
#include "lib/curl/Setup.hxx"
#include "lib/curl/Slist.hxx"

#include <cstdio>
#include <cstring>
#include <ctime>
#include <string_view>
#include <vector>

#include <curl/curl.h>

/* ------------------------------------------------------------------ */
/* Header capture                                                      */
/* ------------------------------------------------------------------ */

/**
 * Captures the Set-Cookie header containing the refresh token. The
 * curl read/header callbacks themselves are now defined inline at the
 * call sites as capture-less lambdas (after migration to CurlEasy
 * wrappers); we only need the captured-state struct here. */
struct HeaderCapture {
  std::string cookie_header;
};

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
/* JSON string escaping                                                */
/* ------------------------------------------------------------------ */

/**
 * Escape a string for embedding in a JSON string literal.
 *
 * Without this, an email/password containing a backslash or double
 * quote would either break the JSON syntax (auth fails) or — in the
 * worst case — let a crafted value inject extra JSON fields into the
 * auth payload.
 */
static std::string
EscapeJsonString(std::string_view s) noexcept
{
  std::string out;
  out.reserve(s.size() + 4);
  for (char c : s) {
    switch (c) {
    case '"':  out += "\\\""; break;
    case '\\': out += "\\\\"; break;
    case '\b': out += "\\b";  break;
    case '\f': out += "\\f";  break;
    case '\n': out += "\\n";  break;
    case '\r': out += "\\r";  break;
    case '\t': out += "\\t";  break;
    default:
      if (static_cast<unsigned char>(c) < 0x20) {
        char buf[8];
        std::snprintf(buf, sizeof(buf), "\\u%04x",
                      static_cast<unsigned>(static_cast<unsigned char>(c)));
        out += buf;
      } else {
        out += c;
      }
    }
  }
  return out;
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

  /* Don't log the email — it identifies the account. Length only. */
  LogFmt("xctherm: authenticating (user len={})", credentials.email.size());

  const std::string post_data =
    R"({"email":")" + EscapeJsonString(credentials.email) +
    R"(","password":")" + EscapeJsonString(credentials.password) + R"("})";

  std::vector<uint8_t> response_buffer;
  HeaderCapture header_capture;

  CURLcode res = CURLE_OK;
  long http_code = 0;
  try {
    CurlEasy easy{"https://xctherm.com/api/accounts/authenticate"};
    Curl::Setup(easy);          // shared CA bundle, User-Agent, NoSignal
    easy.SetTimeout(30);
    easy.SetRequestBody(post_data);

    CurlSlist headers;
    headers.Append("Content-Type: application/json");
    easy.SetRequestHeaders(headers.Get());

    easy.SetWriteFunction(
      [](char *ptr, size_t size, size_t nmemb, void *userdata) -> size_t {
        const size_t total = size * nmemb;
        auto *buf = static_cast<std::vector<uint8_t> *>(userdata);
        buf->insert(buf->end(),
                    reinterpret_cast<uint8_t *>(ptr),
                    reinterpret_cast<uint8_t *>(ptr) + total);
        return total;
      },
      &response_buffer);
    easy.SetHeaderFunction(
      [](char *buffer, size_t size, size_t nitems,
         void *userdata) -> size_t {
        const size_t total = size * nitems;
        std::string line(buffer, total);
        auto *cap = static_cast<HeaderCapture *>(userdata);
        if (line.size() > 12 &&
            (line.substr(0, 11) == "Set-Cookie:" ||
             line.substr(0, 11) == "set-cookie:")) {
          if (line.find("refreshToken") != std::string::npos)
            cap->cookie_header = line.substr(12);
        }
        return total;
      },
      &header_capture);

    easy.Perform();
    easy.GetInfo(CURLINFO_RESPONSE_CODE, &http_code);
  } catch (const std::exception &) {
    res = CURLE_OPERATION_TIMEDOUT;
  }

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

  std::vector<uint8_t> response_buffer;
  HeaderCapture header_capture;

  /* Send refresh token as cookie */
  const std::string cookie = "refreshToken=" + refresh_token;

  CURLcode res = CURLE_OK;
  long http_code = 0;
  try {
    CurlEasy easy{"https://xctherm.com/api/accounts/refresh-token"};
    Curl::Setup(easy);
    easy.SetTimeout(20);
    easy.SetPost();
    easy.SetRequestBody(std::string_view{});
    easy.SetOption(CURLOPT_COOKIE, cookie.c_str());

    CurlSlist headers;
    headers.Append("Content-Type: application/json");
    easy.SetRequestHeaders(headers.Get());

    easy.SetWriteFunction(
      [](char *ptr, size_t size, size_t nmemb, void *userdata) -> size_t {
        const size_t total = size * nmemb;
        auto *buf = static_cast<std::vector<uint8_t> *>(userdata);
        buf->insert(buf->end(),
                    reinterpret_cast<uint8_t *>(ptr),
                    reinterpret_cast<uint8_t *>(ptr) + total);
        return total;
      },
      &response_buffer);
    easy.SetHeaderFunction(
      [](char *buffer, size_t size, size_t nitems,
         void *userdata) -> size_t {
        const size_t total = size * nitems;
        std::string line(buffer, total);
        auto *cap = static_cast<HeaderCapture *>(userdata);
        if (line.size() > 12 &&
            (line.substr(0, 11) == "Set-Cookie:" ||
             line.substr(0, 11) == "set-cookie:")) {
          if (line.find("refreshToken") != std::string::npos)
            cap->cookie_header = line.substr(12);
        }
        return total;
      },
      &header_capture);

    easy.Perform();
    easy.GetInfo(CURLINFO_RESPONSE_CODE, &http_code);
  } catch (const std::exception &) {
    res = CURLE_OPERATION_TIMEDOUT;
  }

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
