// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The XCSoar Project

#pragma once

#include "WifiTypes.hpp"
#include "Language/Language.hpp"

[[gnu::pure]]
static inline const char *
FormatWifiAuthModeLabel(WifiAuthMode mode) noexcept
{
  switch (mode) {
  case WifiAuthMode::Open:
    return _("Open");

  case WifiAuthMode::Passphrase:
    return _("Passphrase");

  case WifiAuthMode::Unsupported:
    return _("Unsupported");

  case WifiAuthMode::COUNT:
    break;
  }

  return _("Unsupported");
}

[[gnu::pure]]
static inline const char *
FormatWifiSecurityLabel(WifiSecurity security) noexcept
{
  return FormatWifiAuthModeLabel(ToWifiAuthMode(security));
}

[[gnu::pure]]
static inline const char *
FormatWifiConnectionStateLabel(WifiConnectionState state) noexcept
{
  switch (state) {
  case WifiConnectionState::Connected:
    return _("Connected");

  case WifiConnectionState::Connecting:
    return _("Connecting");

  case WifiConnectionState::Disconnected:
    return _("Not connected");

  case WifiConnectionState::Unknown:
    return _("Status unknown");

  case WifiConnectionState::COUNT:
    break;
  }

  return _("Status unknown");
}