// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The XCSoar Project

#pragma once

#include "net/client/auth/DeviceFlow.hpp"

#include <cstdint>
#include <string>
#include <string_view>

namespace WeGlide {

/** RFC 8628 client id registered for XCSoar. */
inline constexpr std::string_view kOAuthClientId{"xcsoar"};

/**
 * Scopes for IGC upload, task declaration, and Copilot tiles.
 * See Protocol.md.
 */
inline constexpr std::string_view kOAuthScope{
  "upload declare copilot:layers"};

inline constexpr std::string_view kDeviceAuthorizeUrl{
  "https://api.weglide.org/v1/auth/device/authorize"};

inline constexpr std::string_view kTokenUrl{
  "https://api.weglide.org/v1/auth/token"};

/** Refresh while more than a day of the access token remains. */
inline constexpr uint32_t kAccessRefreshMargin = 24u * 60u * 60u;

[[nodiscard]] inline std::string
AuthorizeForm()
{
  return Net::Client::Auth::DeviceAuthorizeForm(kOAuthClientId, kOAuthScope);
}

} // namespace WeGlide
