// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The XCSoar Project

#include "DeviceFlow.hpp"

#include <boost/json.hpp>

#include <optional>

namespace Net::Client::Auth {

static constexpr std::string_view kDeviceGrant =
  "urn:ietf:params:oauth:grant-type:device_code";

static constexpr unsigned kDefaultInterval = 5;
static constexpr unsigned kSlowDownSeconds = 5;

static uint32_t
AddSeconds(uint32_t now, unsigned seconds) noexcept
{
  const uint64_t sum = uint64_t{now} + seconds;
  return sum > 0xffffffffu ? 0xffffffffu : static_cast<uint32_t>(sum);
}

static void
AppendEncoded(std::string &out, std::string_view value)
{
  static constexpr char hex[] = "0123456789ABCDEF";

  for (const unsigned char c : value) {
    const bool plain = (c >= 'a' && c <= 'z') || (c >= 'A' && c <= 'Z') ||
      (c >= '0' && c <= '9') || c == '-' || c == '.' || c == '_' || c == '~';
    if (plain) {
      out.push_back(static_cast<char>(c));
    } else if (c == ' ') {
      out.push_back('+');
    } else {
      out.push_back('%');
      out.push_back(hex[c >> 4]);
      out.push_back(hex[c & 0x0f]);
    }
  }
}

static void
AppendField(std::string &out, std::string_view name, std::string_view value)
{
  if (!out.empty())
    out.push_back('&');
  AppendEncoded(out, name);
  out.push_back('=');
  AppendEncoded(out, value);
}

static std::optional<std::string>
ReadString(const boost::json::object &object, std::string_view key)
{
  const auto *value = object.if_contains(key);
  if (value == nullptr || !value->is_string())
    return std::nullopt;
  return std::string{value->as_string()};
}

static unsigned
ReadUnsigned(const boost::json::object &object, std::string_view key,
             unsigned fallback)
{
  const auto *value = object.if_contains(key);
  if (value == nullptr)
    return fallback;

  if (const auto i = value->if_int64(); i && *i >= 0)
    return static_cast<unsigned>(*i);
  if (const auto u = value->if_uint64())
    return static_cast<unsigned>(*u);
  if (const auto d = value->if_double(); d && *d >= 0)
    return static_cast<unsigned>(*d);
  return fallback;
}

static std::string_view
ErrorCode(const boost::json::value &body)
{
  if (!body.is_object())
    return {};

  const auto &object = body.as_object();
  if (const auto *error = object.if_contains("error");
      error != nullptr && error->is_string())
    return error->as_string();

  const auto *detail = object.if_contains("detail");
  if (detail == nullptr || !detail->is_object())
    return {};

  const auto *error = detail->as_object().if_contains("error");
  if (error == nullptr || !error->is_string())
    return {};
  return error->as_string();
}

static std::optional<OAuthToken>
ReadToken(const boost::json::value &body)
{
  if (!body.is_object())
    return std::nullopt;

  const auto &object = body.as_object();
  auto access = ReadString(object, "access_token");
  if (!access || access->empty())
    return std::nullopt;

  OAuthToken token;
  token.access_token = std::move(*access);
  if (auto refresh = ReadString(object, "refresh_token"))
    token.refresh_token = std::move(*refresh);
  token.expires_in_seconds = ReadUnsigned(object, "expires_in", 0);
  return token;
}

static std::optional<DeviceCode>
ReadDeviceCode(std::string_view json)
{
  const auto body = boost::json::parse(json);
  if (!body.is_object())
    return std::nullopt;

  const auto &object = body.as_object();
  auto device_code = ReadString(object, "device_code");
  auto user_code = ReadString(object, "user_code");
  auto verification_uri = ReadString(object, "verification_uri");
  if (!device_code || device_code->empty() ||
      !user_code || user_code->empty() ||
      !verification_uri || verification_uri->empty())
    return std::nullopt;

  DeviceCode code;
  code.device_code = std::move(*device_code);
  code.user_code = std::move(*user_code);
  code.verification_uri = std::move(*verification_uri);
  if (auto complete = ReadString(object, "verification_uri_complete"))
    code.verification_uri_complete = std::move(*complete);
  code.expires_in_seconds = ReadUnsigned(object, "expires_in", 0);
  code.interval_seconds = ReadUnsigned(object, "interval", kDefaultInterval);
  if (code.interval_seconds == 0)
    code.interval_seconds = kDefaultInterval;
  return code;
}

void
DeviceFlow::Clear() noexcept
{
  *this = {};
}

DeviceFlowAction
DeviceFlow::Next(uint32_t now, uint32_t refresh_margin_seconds) const noexcept
{
  const bool access_alive = !token.access_token.empty() &&
    access_expires_at > now;
  const uint64_t fresh_until =
    uint64_t{now} + refresh_margin_seconds;
  if (access_alive && access_expires_at > fresh_until)
    return DeviceFlowAction::READY;

  if (!token.refresh_token.empty())
    return DeviceFlowAction::REFRESH;

  if (access_alive)
    return DeviceFlowAction::READY;

  if (!pending.device_code.empty()) {
    if (device_expires_at == 0 || now < device_expires_at)
      return DeviceFlowAction::WAIT;
    return DeviceFlowAction::RESTART;
  }

  return DeviceFlowAction::AUTHORIZE;
}

bool
DeviceFlow::OnAuthorize(std::string_view json, uint32_t now) noexcept
{
  try {
    auto code = ReadDeviceCode(json);
    if (!code)
      return false;

    poll_interval_seconds = code->interval_seconds;
    wait_seconds = poll_interval_seconds;
    device_expires_at = code->expires_in_seconds == 0
      ? 0
      : AddSeconds(now, code->expires_in_seconds);
    pending = std::move(*code);
    return true;
  } catch (...) {
    return false;
  }
}

DeviceFlowAction
DeviceFlow::OnPoll(unsigned http_status, std::string_view json,
                   uint32_t now) noexcept
{
  try {
    const auto body = boost::json::parse(json);
    if (http_status == 200) {
      auto granted = ReadToken(body);
      if (!granted)
        return DeviceFlowAction::RETRY;

      access_expires_at = granted->expires_in_seconds == 0
        ? 0
        : AddSeconds(now, granted->expires_in_seconds);
      token = std::move(*granted);
      pending = {};
      device_expires_at = 0;
      wait_seconds = 0;
      return DeviceFlowAction::READY;
    }

    const auto error = ErrorCode(body);
    if (error == "authorization_pending") {
      wait_seconds = poll_interval_seconds;
      return DeviceFlowAction::WAIT;
    }
    if (error == "slow_down") {
      poll_interval_seconds += kSlowDownSeconds;
      if (pending.interval_seconds != 0)
        pending.interval_seconds = poll_interval_seconds;
      wait_seconds = poll_interval_seconds;
      return DeviceFlowAction::WAIT;
    }
    if (error == "expired_token") {
      pending = {};
      device_expires_at = 0;
      return DeviceFlowAction::RESTART;
    }
    if (error == "access_denied") {
      pending = {};
      device_expires_at = 0;
      return DeviceFlowAction::DENIED;
    }
    return DeviceFlowAction::RETRY;
  } catch (...) {
    return DeviceFlowAction::RETRY;
  }
}

DeviceFlowAction
DeviceFlow::OnRefresh(unsigned http_status, std::string_view json,
                      uint32_t now) noexcept
{
  try {
    if (http_status != 200) {
      /* Only invalid_grant means this refresh token is dead.
         Other 400 and 401 codes are request errors; keep the token. */
      bool invalid_grant = false;
      try {
        const auto parsed = boost::json::parse(json);
        invalid_grant = ErrorCode(parsed) == "invalid_grant";
      } catch (...) {
      }
      if (invalid_grant) {
        token.refresh_token.clear();
        return DeviceFlowAction::AUTHORIZE;
      }
      return DeviceFlowAction::RETRY;
    }

    const auto body = boost::json::parse(json);
    auto granted = ReadToken(body);
    if (!granted)
      return DeviceFlowAction::RETRY;

    /* The previous refresh token is invalid once a new one arrives.
       A success that omits it cannot be used again either. */
    access_expires_at = granted->expires_in_seconds == 0
      ? 0
      : AddSeconds(now, granted->expires_in_seconds);
    token = std::move(*granted);
    return DeviceFlowAction::READY;
  } catch (...) {
    return DeviceFlowAction::RETRY;
  }
}

std::string
DeviceAuthorizeForm(std::string_view client_id, std::string_view scope)
{
  std::string body;
  AppendField(body, "client_id", client_id);
  AppendField(body, "scope", scope);
  return body;
}

std::string
DevicePollForm(std::string_view client_id, std::string_view device_code)
{
  std::string body;
  AppendField(body, "client_id", client_id);
  AppendField(body, "grant_type", kDeviceGrant);
  AppendField(body, "device_code", device_code);
  return body;
}

std::string
RefreshForm(std::string_view client_id, std::string_view refresh_token)
{
  std::string body;
  AppendField(body, "client_id", client_id);
  AppendField(body, "grant_type", "refresh_token");
  AppendField(body, "refresh_token", refresh_token);
  return body;
}

} // namespace Net::Client::Auth
