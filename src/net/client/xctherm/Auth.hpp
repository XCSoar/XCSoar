// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The XCSoar Project

#pragma once

#include "net/client/auth/JwtBearerSession.hpp"

namespace XCTherm {

inline constexpr Net::Client::Auth::JwtBearerSessionConfig kAuthConfig{
  .log_tag = "xctherm",
  .login_url = "https://xctherm.com/api/accounts/authenticate",
  .refresh_url = "https://xctherm.com/api/accounts/refresh-token",
  .access_token_json_field = "jwtToken",
  .login_id_json_field = "email",
  .login_secret_json_field = "password",
  .refresh_cookie_name = "refreshToken",
  .token_lifetime_seconds = 14 * 60,
};

using Auth = Net::Client::Auth::JwtBearerSession;

} // namespace XCTherm
