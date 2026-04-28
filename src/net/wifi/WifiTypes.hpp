// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The XCSoar Project

#pragma once

enum class WifiSecurity {
  Unknown,
  WPA,
  WEP,
  Open,
  COUNT,
};

enum class WifiAuthMode {
  Open,
  Passphrase,
  Unsupported,
  COUNT,
};

enum class WifiSignalUnit {
  Unknown,
  Dbm,
  Relative,
  COUNT,
};

enum class WifiConnectionState {
  Unknown,
  Disconnected,
  Connecting,
  Connected,
  COUNT,
};

enum class WifiNetworkKind {
  VisibleAccessPoint,
  SavedProfile,
  ConnectedNetwork,
  COUNT,
};

[[gnu::const]]
static constexpr WifiAuthMode
ToWifiAuthMode(WifiSecurity security) noexcept
{
  switch (security) {
  case WifiSecurity::WPA:
  case WifiSecurity::WEP:
    return WifiAuthMode::Passphrase;

  case WifiSecurity::Open:
    return WifiAuthMode::Open;

  case WifiSecurity::Unknown:
  case WifiSecurity::COUNT:
    return WifiAuthMode::Unsupported;
  }

  return WifiAuthMode::Unsupported;
}