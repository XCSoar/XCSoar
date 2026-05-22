// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The XCSoar Project

#include "XCThermAuth.hpp"
#include "LogFile.hpp"
#include "Weather/xctherm/Http.hpp"
#include "lib/curl/Slist.hxx"

#include <cstring>
#include <ctime>
#include <exception>
#include <string_view>

#include <boost/json.hpp>

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

  const boost::json::object body{
    {"email", credentials.email},
    {"password", credentials.password},
  };
  const std::string post_data = boost::json::serialize(body);

  CurlSlist headers;
  headers.Append("Content-Type: application/json");

  XCThermHttp::Response response;
  if (!XCThermHttp::PerformPost(
        "https://xctherm.com/api/accounts/authenticate",
        headers.Get(), post_data, response, true) ||
      response.http_code != 200) {
    LogFmt("xctherm: auth failed http={}", response.http_code);
    return false;
  }

  /* Parse JWT */
  if (!ParseJWTFromResponse(response.body, jwt_token)) {
    LogFmt("xctherm: no jwtToken in auth response");
    return false;
  }

  jwt_expiry = static_cast<uint32_t>(std::time(nullptr)) + JWT_LIFETIME_SECONDS;

  /* Parse refresh token from cookie */
  if (!response.refresh_cookie_line.empty()) {
    ParseRefreshTokenFromCookie(response.refresh_cookie_line, refresh_token);
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

  const std::string cookie = "refreshToken=" + refresh_token;

  CurlSlist headers;
  headers.Append("Content-Type: application/json");

  XCThermHttp::Response response;
  if (!XCThermHttp::PerformPost(
        "https://xctherm.com/api/accounts/refresh-token",
        headers.Get(), "", response, true, cookie.c_str()) ||
      response.http_code != 200) {
    LogFmt("xctherm: refresh failed http={}", response.http_code);
    refresh_token.clear();
    return false;
  }

  if (!ParseJWTFromResponse(response.body, jwt_token)) {
    LogFmt("xctherm: no jwtToken in refresh response");
    return false;
  }

  jwt_expiry = static_cast<uint32_t>(std::time(nullptr)) + JWT_LIFETIME_SECONDS;

  if (!response.refresh_cookie_line.empty()) {
    std::string new_refresh;
    if (ParseRefreshTokenFromCookie(response.refresh_cookie_line, new_refresh))
      refresh_token = std::move(new_refresh);
  }

  LogFmt("xctherm: refresh success, jwt_len={}", jwt_token.size());
  return true;
}

/* ------------------------------------------------------------------ */
/* JSON/Cookie parsing                                                 */
/* ------------------------------------------------------------------ */

bool
XCThermAuth::ParseJWTFromResponse(const std::string &response,
                                  std::string &out_token) noexcept
{
  try {
    const boost::json::value jv = boost::json::parse(response);
    if (!jv.is_object())
      return false;

    const auto *token = jv.as_object().if_contains("jwtToken");
    if (token == nullptr || !token->is_string())
      return false;

    out_token = std::string{token->as_string()};
    return !out_token.empty();
  } catch (const std::exception &) {
    return false;
  }
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