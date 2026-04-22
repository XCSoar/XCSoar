// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The XCSoar Project

#pragma once

#include "lib/dbus/Connection.hxx"

#include <string>
#include <vector>

namespace CmWifi {

struct ServiceEntry {
	std::string path;
	/** ESSID for display. */
	std::string ssid_text;
	std::string state;
	bool needs_key{false};
	int strength{0};
};

/**
 * ConnMan service "State" values that mean the WiFi service is in use
 * (online, ready, configuration).
 */
bool
IsActiveServiceState(const std::string &state) noexcept;

std::vector<ServiceEntry>
ListServices(ODBus::Connection &c);

std::string
FormatStatus(ODBus::Connection &c);

void
SetPassphrase(ODBus::Connection &c, const char *path, const char *passphrase);

void
Connect(ODBus::Connection &c, const char *path);

/**
 * Poll the service `State` until `online` or `ready`, on failure, or timeout.
 * Call after `Connect`; `std::exception::what()` is mapped in
 * `FormatWifiErrorForUser`.
 */
void
WaitForServiceConnected(ODBus::Connection &c, const char *path);

void
Disconnect(ODBus::Connection &c, const char *path);

} // namespace CmWifi
