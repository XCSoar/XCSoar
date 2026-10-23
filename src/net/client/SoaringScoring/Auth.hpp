// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The XCSoar Project

#pragma once

#include "net/client/auth/DeviceFlow.hpp"

#include <string>
#include <string_view>

namespace SoaringScoring {

/** Public OAuth client id. It identifies the app and is not a secret. */
inline constexpr std::string_view kOAuthClientId{"xcsoar"};

inline constexpr std::string_view kOAuthScope{
  "tasks:read flights:write"};

inline constexpr std::string_view kOrigin{
  "https://soaringscoring.com"};

inline constexpr std::string_view kDeviceAuthorizeUrl{
  "https://soaringscoring.com/api/v1/oauth/device_authorization"};

inline constexpr std::string_view kTokenUrl{
  "https://soaringscoring.com/api/v1/oauth/token"};

inline constexpr std::string_view kRevokeUrl{
  "https://soaringscoring.com/api/v1/oauth/revoke"};

inline constexpr std::string_view kEntriesUrl{
  "https://soaringscoring.com/api/v1/public/me/entries"};

[[nodiscard]] inline std::string
AuthorizeForm()
{
  return Net::Client::Auth::DeviceAuthorizeForm(kOAuthClientId,
                                                kOAuthScope);
}

} // namespace SoaringScoring
