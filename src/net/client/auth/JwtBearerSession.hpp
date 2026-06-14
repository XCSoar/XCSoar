// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The XCSoar Project

#pragma once

#include <cstdint>
#include <string>

namespace Net::Client::Auth {

/**
 * Configuration for email/password login with JWT access token and
 * optional refresh token delivered as an HTTP cookie.
 */
struct JwtBearerSessionConfig {
  /** Prefix for LogFmt messages (e.g. "xctherm"). */
  const char *log_tag;

  const char *login_url;
  const char *refresh_url;

  /** JSON field name for the access token in login/refresh responses. */
  const char *access_token_json_field;

  /** JSON object keys for the login POST body. */
  const char *login_id_json_field;
  const char *login_secret_json_field;

  /** Cookie name for the refresh token (without trailing '='). */
  const char *refresh_cookie_name;

  /** Conservative lifetime before EnsureValidToken() re-authenticates. */
  uint32_t token_lifetime_seconds;
};

/**
 * JWT bearer session: login, refresh via cookie, Bearer header for API calls.
 */
class JwtBearerSession {
public:
  explicit JwtBearerSession(const JwtBearerSessionConfig &config) noexcept
    :config(config) {}

  void SetCredentials(const std::string &id,
                      const std::string &secret) noexcept;

  bool EnsureValidToken() noexcept;
  bool ForceReauthenticate() noexcept;

  bool HasValidToken() const noexcept;

  /** Returns "Authorization: Bearer <token>". */
  std::string GetAuthHeader() const noexcept;

private:
  const JwtBearerSessionConfig &config;

  std::string login_id;
  std::string login_secret;

  std::string access_token;
  std::string refresh_token;
  uint32_t access_token_expiry = 0;

  bool Authenticate() noexcept;
  bool RefreshAccessToken() noexcept;

  static bool ParseAccessTokenFromResponse(const JwtBearerSessionConfig &config,
                                           const std::string &response,
                                           std::string &out_token) noexcept;

  static bool ParseRefreshTokenFromCookie(const JwtBearerSessionConfig &config,
                                          const std::string &cookie_header,
                                          std::string &out_token) noexcept;
};

} // namespace Net::Client::Auth
