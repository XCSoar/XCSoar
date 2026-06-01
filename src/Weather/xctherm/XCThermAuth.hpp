// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The XCSoar Project

#pragma once

#include <cstdint>
#include <string>

/**
 * XCTherm JWT authentication with refresh-token support.
 *
 * Flow:
 *  1. Login with email/password → jwtToken (15 min) + refreshToken (cookie)
 *  2. On expiry, use refreshToken to obtain a new jwtToken
 *  3. If refresh fails, fall back to full login
 */
class XCThermAuth {
public:
  struct Credentials {
    std::string email;
    std::string password;
  };

  void SetCredentials(const std::string &email,
                      const std::string &password) noexcept;

  /**
   * Ensure we have a valid JWT token. Refreshes or re-authenticates
   * as needed. Returns true if a valid token is available.
   */
  bool EnsureValidToken() noexcept;

  /**
   * Force a full re-authentication (e.g. after 401 from API).
   */
  bool ForceReauthenticate() noexcept;

  bool HasValidToken() const noexcept;

  /**
   * Build the "Authorization: Bearer <token>" header value.
   */
  std::string GetAuthHeader() const noexcept;

private:
  Credentials credentials;

  std::string jwt_token;
  std::string refresh_token;
  uint32_t jwt_expiry = 0;  // unix timestamp when JWT expires

  static constexpr uint32_t JWT_LIFETIME_SECONDS = 14 * 60; // 14 min (conservative)

  bool Authenticate() noexcept;
  bool RefreshJWT() noexcept;

  /**
   * Parse jwtToken from JSON response body.
   */
  static bool ParseJWTFromResponse(const std::string &response,
                                   std::string &out_token) noexcept;

  /**
   * Parse refreshToken from Set-Cookie header value.
   */
  static bool ParseRefreshTokenFromCookie(const std::string &cookie_header,
                                          std::string &out_token) noexcept;
};
