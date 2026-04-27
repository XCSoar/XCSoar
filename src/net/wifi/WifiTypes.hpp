// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The XCSoar Project

#pragma once

#include "util/StaticString.hxx"

enum class WifiSecurity {
  Unknown,
  WPA,
  WEP,
  Open,
};

enum class WifiAuthMode {
  Open,
  Passphrase,
  Unsupported,
};

enum class WifiSignalUnit {
  Unknown,
  Dbm,
  Relative,
};

enum class WifiConnectionState {
  Unknown,
  Disconnected,
  Connecting,
  Connected,
};

enum class WifiNetworkKind {
  VisibleAccessPoint,
  SavedProfile,
  ConnectedNetwork,
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
    return WifiAuthMode::Unsupported;
  }

  return WifiAuthMode::Unsupported;
}

struct WifiVisibleNetwork {
  StaticString<32> bssid;
  StaticString<256> ssid;
  signed signal_level;
  WifiSecurity security = WifiSecurity::Unknown;
};

struct WifiConfiguredNetworkInfo {
  unsigned id;
  StaticString<256> ssid;
  StaticString<32> bssid;
};

struct WifiBackendStatus {
  WifiConnectionState state = WifiConnectionState::Unknown;
  WifiSignalUnit signal_unit = WifiSignalUnit::Unknown;
  StaticString<32> interface_name;
  StaticString<32> bssid;
  StaticString<256> ssid;

  void Clear() {
    state = WifiConnectionState::Unknown;
    signal_unit = WifiSignalUnit::Unknown;
    interface_name.clear();
    bssid.clear();
    ssid.clear();
  }
};

struct WifiNetworkEntry {
  StaticString<256> profile_id;
  StaticString<32> bssid;
  StaticString<256> ssid;
  signed signal_level = 0;
  WifiSecurity security = WifiSecurity::Unknown;
  WifiSignalUnit signal_unit = WifiSignalUnit::Unknown;
  WifiNetworkKind kind = WifiNetworkKind::VisibleAccessPoint;
  bool is_visible = false;
  bool can_connect = false;
  bool can_disconnect = false;
  bool can_forget = false;

  void Clear() {
    profile_id.clear();
    bssid.clear();
    ssid.clear();
    signal_level = 0;
    security = WifiSecurity::Unknown;
    signal_unit = WifiSignalUnit::Unknown;
    kind = WifiNetworkKind::VisibleAccessPoint;
    is_visible = false;
    can_connect = false;
    can_disconnect = false;
    can_forget = false;
  }
};

struct WifiConnectRequest {
  StaticString<256> profile_id;
  StaticString<256> ssid;
  StaticString<64> secret;
  WifiSecurity security = WifiSecurity::Unknown;

  void Clear() {
    profile_id.clear();
    ssid.clear();
    secret.clear();
    security = WifiSecurity::Unknown;
  }
};