// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The XCSoar Project

#pragma once

#include <string>

/**
 * Minimal XCTherm authentication result.
 */
struct XCThermAuthResult {
  bool success = false;
  std::string jwt_token;
  std::string error_message;
};

/**
 * Authenticate against the XCTherm API.
 *
 * @param email User email
 * @param password User password
 * @return Authentication result with JWT token on success
 */
XCThermAuthResult
XCThermAuthenticate(const std::string &email,
                    const std::string &password) noexcept;
