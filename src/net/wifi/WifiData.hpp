// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The XCSoar Project

#pragma once

#include "WifiTypes.hpp"
#include "WifiFormat.hpp"
#include "Language/Language.hpp"
#include "net/IPv4Address.hxx"
#include "util/StaticString.hxx"

static inline bool
TryFormatWifiInterfaceAddress(const char *interface_name,
                              StaticString<64> &text)
{
  if (interface_name == nullptr || *interface_name == 0)
    return false;

#if !defined(_WIN32) && !defined(__BIONIC__)
  const auto addr = IPv4Address::GetDeviceAddress(interface_name);
  return addr.IsDefined() &&
    addr.ToString(text.buffer(), text.capacity()) != nullptr;
#else
  (void)text;
  return false;
#endif
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

  static StaticString<256> Format(const WifiBackendStatus &status) {
    StaticString<256> text;

    switch (status.state) {
    case WifiConnectionState::Connected:
      if (!status.ssid.empty())
        text.Format(_("Connected to %s"), status.ssid.c_str());
      else
        text = FormatWifiConnectionStateLabel(status.state);
      break;

    case WifiConnectionState::Connecting:
    case WifiConnectionState::Disconnected:
    case WifiConnectionState::Unknown:
    case WifiConnectionState::COUNT:
      text = FormatWifiConnectionStateLabel(status.state);
      break;
    }

    return text;
  }

  static StaticString<64> FormatIpAddress(const WifiBackendStatus &status) {
    StaticString<64> text;

    if (status.state != WifiConnectionState::Connected) {
      text = FormatWifiConnectionStateLabel(status.state);
      return text;
    }

    if (status.interface_name.empty()) {
      text = _("Unknown");
      return text;
    }

#if !defined(_WIN32) && !defined(__BIONIC__)
    if (!TryFormatWifiInterfaceAddress(status.interface_name.c_str(), text))
      text = _("Unknown");
#else
    text = _("Unknown");
#endif

    return text;
  }

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
  StaticString<32> interface_name;
  StaticString<256> ssid;
  signed signal_level = 0;
  WifiSecurity security = WifiSecurity::Unknown;
  WifiSignalUnit signal_unit = WifiSignalUnit::Unknown;
  WifiNetworkKind kind = WifiNetworkKind::VisibleAccessPoint;
  bool is_visible = false;
  bool has_stored_credentials = false;
  bool can_connect = false;
  bool can_disconnect = false;
  bool can_forget = false;

  static StaticString<64> FormatState(const WifiNetworkEntry &entry) {
    StaticString<64> text;

    if (entry.kind == WifiNetworkKind::ConnectedNetwork) {
      if (!entry.interface_name.empty()) {
        StaticString<64> address;
        if (TryFormatWifiInterfaceAddress(entry.interface_name.c_str(),
                                          address)) {
          text.Format("%s (%s)", _("Connected"), address.c_str());
          return text;
        }
      }

      text = _("Connected");
      return text;
    }

    if (entry.has_stored_credentials) {
      if (entry.is_visible)
        text = _("Saved, available");
      else
        text = _("Saved");

      return text;
    }

    if (entry.is_visible)
      text = _("Available");

    return text;
  }

  void Clear() {
    profile_id.clear();
    bssid.clear();
    interface_name.clear();
    ssid.clear();
    signal_level = 0;
    security = WifiSecurity::Unknown;
    signal_unit = WifiSignalUnit::Unknown;
    kind = WifiNetworkKind::VisibleAccessPoint;
    is_visible = false;
    has_stored_credentials = false;
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