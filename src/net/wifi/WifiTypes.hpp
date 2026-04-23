// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The XCSoar Project

#pragma once

#include "util/StaticString.hxx"

enum WifiSecurity {
  WPA_SECURITY,
  WEP_SECURITY,
  OPEN_SECURITY,
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

[[gnu::const]]
static constexpr WifiAuthMode
ToWifiAuthMode(WifiSecurity security) noexcept
{
  switch (security) {
  case WPA_SECURITY:
  case WEP_SECURITY:
    return WifiAuthMode::Passphrase;

  case OPEN_SECURITY:
    return WifiAuthMode::Open;
  }

  return WifiAuthMode::Unsupported;
}

struct WifiStatus {
  StaticString<32> bssid;
  StaticString<256> ssid;

  void Clear() {
    bssid.clear();
    ssid.clear();
  }
};

struct WifiVisibleNetwork {
  StaticString<32> bssid;
  StaticString<256> ssid;
  signed signal_level;
  enum WifiSecurity security;
};

struct WifiConfiguredNetworkInfo {
  int id;
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
  WifiAuthMode auth = WifiAuthMode::Unsupported;
  WifiSignalUnit signal_unit = WifiSignalUnit::Unknown;
  bool is_visible = false;
  bool is_saved = false;
  bool is_connected = false;

  void Clear() {
    profile_id.clear();
    bssid.clear();
    ssid.clear();
    signal_level = 0;
    auth = WifiAuthMode::Unsupported;
    signal_unit = WifiSignalUnit::Unknown;
    is_visible = false;
    is_saved = false;
    is_connected = false;
  }
};

struct WifiConnectRequest {
  StaticString<256> profile_id;
  StaticString<256> ssid;
  StaticString<64> secret;
  WifiAuthMode auth = WifiAuthMode::Unsupported;

  void Clear() {
    profile_id.clear();
    ssid.clear();
    secret.clear();
    auth = WifiAuthMode::Unsupported;
  }
};