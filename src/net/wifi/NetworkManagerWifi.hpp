// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The XCSoar Project

#pragma once

#include "lib/dbus/Connection.hxx"

#include <cstddef>
#include <cstdint>
#include <string>
#include <vector>

namespace NmWifi {

struct AccessPoint {
	std::string ssid_text;
	/** D-Bus path of the AccessPoint (can change when NM rescans new objects). */
	std::string ap_path;
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

/**
 * True if the NetworkManager D-Bus name is on the bus (probed elsewhere).
 * Find a WiFi device, may throw.
 */
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

/**
 * D-Bus object path of the current AP on the wireless device, or empty
 * if none.  Used to mark the active network in a scan list.
 */
std::string
GetActiveAccessPointPath(ODBus::Connection &c, const char *wifi_device);

void
ConnectToAp(ODBus::Connection &c, const char *wifi_device, const AccessPoint &ap,
	    const char *wpa2_psk_or_null);

/**
 * True if #Settings has a stored WiFi profile (802-11-wireless) with this
 * SSID, so #ConnectToAp can use #ActivateConnection without a new PSK.
 */
bool
HasSavedConnectionForSsid(ODBus::Connection &c, const std::string &ssid) noexcept;

void
Disconnect(ODBus::Connection &c, const char *wifi_device);

/**
 * Block until the WiFi device is not associated, or a timeout, so
 * a previous network’s autoconnect does not win over the next
 * #ConnectToAp.
 */
void
WaitUntilWifiDisconnected(ODBus::Connection &c, const char *wifi_device);

bool
IsSameBssidAsActive(ODBus::Connection &c, const char *active_access_point_path,
		    const AccessPoint &target) noexcept;

} // namespace NmWifi
