// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The XCSoar Project

#include "JwtBearerSession.hpp"
#include "net/client/SyncHttp.hpp"
#include "LogFile.hpp"
#include "io/StringOutputStream.hxx"
#include "json/Serialize.hxx"
#include "lib/curl/Slist.hxx"

#include <cstring>
#include <ctime>
#include <exception>

#include <boost/json.hpp>

namespace Net::Client::Auth {

void
JwtBearerSession::SetCredentials(const std::string &id,
                                 const std::string &secret) noexcept
{
  if (login_id != id || login_secret != secret) {
    login_id = id;
    login_secret = secret;
    access_token.clear();
    refresh_token.clear();
    access_token_expiry = 0;
  }
}

bool
JwtBearerSession::HasValidToken() const noexcept
{
  return !access_token.empty() &&
         access_token_expiry > static_cast<uint32_t>(std::time(nullptr));
}

std::string
JwtBearerSession::GetAuthHeader() const noexcept
{
  return std::string{"Authorization: Bearer "} + access_token;
}

bool
JwtBearerSession::EnsureValidToken() noexcept
{
  if (HasValidToken())
    return true;

  if (!refresh_token.empty()) {
    if (RefreshAccessToken())
      return true;
    LogFmt("{}: refresh failed, falling back to full auth", config.log_tag);
  }

  return Authenticate();
}

bool
JwtBearerSession::ForceReauthenticate() noexcept
{
  access_token.clear();
  access_token_expiry = 0;
  return Authenticate();
}

bool
JwtBearerSession::Authenticate() noexcept
{
  try {
    if (login_id.empty() || login_secret.empty()) {
      LogFmt("{}: auth skipped — no credentials", config.log_tag);
      return false;
    }

    LogFmt("{}: authenticating (user len={})", config.log_tag, login_id.size());

    const boost::json::object body{
      {config.login_id_json_field, login_id},
      {config.login_secret_json_field, login_secret},
    };
    StringOutputStream post_stream;
    Json::Serialize(post_stream, body);
    const std::string post_data = post_stream.GetValue();

    CurlSlist headers;
    headers.Append("Content-Type: application/json");

    SyncHttpResponse response;
    if (!SyncHttpPost(config.login_url, headers.Get(), post_data, response,
                      config.refresh_cookie_name) ||
        response.http_code != 200) {
      LogFmt("{}: auth failed http={}", config.log_tag, response.http_code);
      return false;
    }

    if (!ParseAccessTokenFromResponse(config, response.body, access_token)) {
      LogFmt("{}: no {} in auth response",
             config.log_tag, config.access_token_json_field);
      return false;
    }

    access_token_expiry = static_cast<uint32_t>(std::time(nullptr)) +
      config.token_lifetime_seconds;

    if (!response.captured_set_cookie.empty()) {
      ParseRefreshTokenFromCookie(config, response.captured_set_cookie,
                                refresh_token);
      if (!refresh_token.empty())
        LogFmt("{}: got {} (len={})", config.log_tag,
               config.refresh_cookie_name, refresh_token.size());
    }

    LogFmt("{}: auth success, token_len={}", config.log_tag,
           access_token.size());
    return true;
  } catch (const std::exception &e) {
    LogFmt("{}: auth exception: {}", config.log_tag, e.what());
    return false;
  } catch (...) {
    LogFmt("{}: auth unknown exception", config.log_tag);
    return false;
  }
}

bool
JwtBearerSession::RefreshAccessToken() noexcept
{
  try {
    if (refresh_token.empty())
      return false;

    LogFmt("{}: refreshing access token", config.log_tag);

    std::string cookie = config.refresh_cookie_name;
    cookie += '=';
    cookie += refresh_token;

    CurlSlist headers;
    headers.Append("Content-Type: application/json");

    SyncHttpResponse response;
    if (!SyncHttpPost(config.refresh_url, headers.Get(), "", response,
                      config.refresh_cookie_name, cookie.c_str()) ||
        response.http_code != 200) {
      LogFmt("{}: refresh failed http={}", config.log_tag, response.http_code);
      refresh_token.clear();
      return false;
    }

    if (!ParseAccessTokenFromResponse(config, response.body, access_token)) {
      LogFmt("{}: no {} in refresh response",
             config.log_tag, config.access_token_json_field);
      return false;
    }

    access_token_expiry = static_cast<uint32_t>(std::time(nullptr)) +
      config.token_lifetime_seconds;

    if (!response.captured_set_cookie.empty()) {
      std::string new_refresh;
      if (ParseRefreshTokenFromCookie(config, response.captured_set_cookie,
                                      new_refresh))
        refresh_token = std::move(new_refresh);
    }

    LogFmt("{}: refresh success, token_len={}",
           config.log_tag, access_token.size());
    return true;
  } catch (const std::exception &e) {
    LogFmt("{}: refresh exception: {}", config.log_tag, e.what());
    return false;
  } catch (...) {
    LogFmt("{}: refresh unknown exception", config.log_tag);
    return false;
  }
}

bool
JwtBearerSession::ParseAccessTokenFromResponse(
  const JwtBearerSessionConfig &config,
  const std::string &response,
  std::string &out_token) noexcept
{
  try {
    const boost::json::value jv = boost::json::parse(response);
    if (!jv.is_object())
      return false;

    const auto *token = jv.as_object().if_contains(config.access_token_json_field);
    if (token == nullptr || !token->is_string())
      return false;

    out_token = std::string{token->as_string()};
    return !out_token.empty();
  } catch (const std::exception &) {
    return false;
  }
}

bool
JwtBearerSession::ParseRefreshTokenFromCookie(
  const JwtBearerSessionConfig &config,
  const std::string &cookie_header,
  std::string &out_token) noexcept
{
  std::string needle = config.refresh_cookie_name;
  needle += '=';

  size_t pos = cookie_header.find(needle);
  if (pos == std::string::npos)
    return false;

  pos += needle.size();
  size_t end = cookie_header.find(';', pos);
  if (end == std::string::npos)
    end = cookie_header.size();

  while (end > pos && (cookie_header[end - 1] == ' ' ||
                       cookie_header[end - 1] == '\r' ||
                       cookie_header[end - 1] == '\n'))
    --end;

  out_token = cookie_header.substr(pos, end - pos);
  return !out_token.empty();
}

} // namespace Net::Client::Auth
