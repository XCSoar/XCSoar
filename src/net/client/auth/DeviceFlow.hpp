// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The XCSoar Project

#pragma once

#include <cstdint>
#include <string>
#include <string_view>

namespace Net::Client::Auth {

/**
 * One step of an RFC 8628 device-authorization grant.
 *
 * The caller performs the HTTP request, then folds the response back
 * in. This type does not open a connection and does not block.
 */
enum class DeviceFlowAction : uint8_t {
  /** POST the authorize form. No device code yet. */
  AUTHORIZE,
  /** Show the user code, wait #DeviceFlow::wait_seconds, then poll. */
  WAIT,
  /** POST the refresh form. The access token is due. */
  REFRESH,
  /** #OAuthToken::access_token may be sent. */
  READY,
  /** The pilot denied this device code. The pending code is cleared. */
  DENIED,
  /** This device code has expired. The pending code is cleared. */
  RESTART,
  /**
   * The response was not one of the RFC 8628 results. The pending
   * code and tokens are unchanged; repeat the request after
   * #DeviceFlow::wait_seconds.
   */
  RETRY,
};

/**
 * What the authorization server returns from the device-authorize
 * request, plus the poll interval the workflow is currently using.
 */
struct DeviceCode {
  std::string device_code;
  std::string user_code;
  std::string verification_uri;
  std::string verification_uri_complete;
  unsigned expires_in_seconds = 0;
  unsigned interval_seconds = 5;
};

/** A bearer token from the token endpoint. */
struct OAuthToken {
  std::string access_token;
  std::string refresh_token;
  unsigned expires_in_seconds = 0;
};

/**
 * Device-flow session.
 *
 * Times are Unix seconds. An expiry of 0 means "not set".
 * #Next() refreshes while the access token is inside
 * @p refresh_margin_seconds of its expiry, and keeps using it when
 * no refresh token is stored.
 */
struct DeviceFlow {
  DeviceCode pending{};
  OAuthToken token{};
  uint32_t access_expires_at = 0;
  uint32_t device_expires_at = 0;
  unsigned poll_interval_seconds = 5;
  unsigned wait_seconds = 0;

  [[nodiscard]] DeviceFlowAction
  Next(uint32_t now, uint32_t refresh_margin_seconds) const noexcept;

  /** Replace #pending from an authorize response. */
  bool OnAuthorize(std::string_view json, uint32_t now) noexcept;

  /**
   * Fold one poll of the token endpoint. A grant replaces #token,
   * including the refresh token, and clears #pending.
   */
  DeviceFlowAction OnPoll(unsigned http_status, std::string_view json,
                          uint32_t now) noexcept;

  /**
   * Fold a refresh response. Success replaces both tokens. HTTP 400
   * and 401 drop the refresh token.
   */
  DeviceFlowAction OnRefresh(unsigned http_status, std::string_view json,
                             uint32_t now) noexcept;

  void Clear() noexcept;
};

/** `client_id` and `scope`, form-urlencoded. */
[[nodiscard]] std::string
DeviceAuthorizeForm(std::string_view client_id,
                    std::string_view scope);

/** `client_id`, the device-code grant type, and `device_code`. */
[[nodiscard]] std::string
DevicePollForm(std::string_view client_id, std::string_view device_code);

/** `client_id`, `grant_type=refresh_token`, and `refresh_token`. */
[[nodiscard]] std::string
RefreshForm(std::string_view client_id, std::string_view refresh_token);

} // namespace Net::Client::Auth
