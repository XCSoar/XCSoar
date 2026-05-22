// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The XCSoar Project

#pragma once

#include "lib/dbus/Connection.hxx"

#include <cstddef>
#include <cstdint>
#include <string>
#include <vector>

namespace NmClient {

struct AccessPoint {
  std::string ssid_text;
  /** D-Bus path of the AccessPoint (can change when NM rescans new objects). */
  std::string ap_path;
  /** Matching saved NetworkManager connection path, if one is known. */
  std::string saved_path;
  /**
   * "HwAddress" from the AP object (BSSID, e.g. "AA:BB:.."); stable for a
   * cell, used to match a user’s selection when #ap_path changes.
   */
  std::string hw_address;
  /** 0-100, best effort from NM. */
  int strength{0};
  /** Need WPA(2) PSK or similar. */
  bool needs_key{false};
};

struct SavedConnection {
  std::string path;
  std::string ssid_text;
  std::string connection_id;
};

std::string
FindWifiDevice(ODBus::Connection &c);

void
SetWirelessEnabled(ODBus::Connection &c, bool on);

void
RequestScan(ODBus::Connection &c, const char *wifi_device);

std::vector<AccessPoint>
ListAccessPoints(ODBus::Connection &c, const char *wifi_device);

std::string
FormatStatus(ODBus::Connection &c, const char *wifi_device);

std::string
GetActiveAccessPointPath(ODBus::Connection &c, const char *wifi_device);

std::vector<SavedConnection>
ListSavedConnections(ODBus::Connection &c);

void
Connect(ODBus::Connection &c, const char *wifi_device, const AccessPoint &ap,
        const char *wpa2_psk_or_null);

void
ConnectSaved(ODBus::Connection &c, const char *wifi_device,
       const char *connection_path, const char *ap_path = nullptr);

bool
HasSavedConnectionForSsid(ODBus::Connection &c, const std::string &ssid) noexcept;

/**
 * Returns the D-Bus path of the first saved Wi-Fi connection with @p ssid,
 * or an empty string if none exists.
 */
std::string
FindSavedConnectionPathForSsid(ODBus::Connection &c, const std::string &ssid);

void
Remove(ODBus::Connection &c, const char *connection_path);

void
Disconnect(ODBus::Connection &c, const char *wifi_device);

void
WaitUntilWifiDisconnected(ODBus::Connection &c, const char *wifi_device);

bool
IsSameBssidAsActive(ODBus::Connection &c, const char *active_access_point_path,
                    const AccessPoint &target) noexcept;

} // namespace NmClient
