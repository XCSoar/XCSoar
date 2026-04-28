// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The XCSoar Project

#pragma once

#include "lib/dbus/Connection.hxx"

#include <string>
#include <vector>

namespace CmClient {

struct ServiceEntry {
  std::string path;
  /** ESSID for display. */
  std::string ssid_text;
  std::string interface_name;
  std::string state;
  bool favorite{false};
  bool immutable{false};
  bool has_strength{false};
  bool needs_key{false};
  int strength{0};
};

bool
IsActiveServiceState(const std::string &state) noexcept;

bool
HasWifiTechnology(ODBus::Connection &c);

bool
IsWifiTechnologyPowered(ODBus::Connection &c);

void
SetWifiTechnologyPowered(ODBus::Connection &c, bool enabled);

void
EnableWifiTechnology(ODBus::Connection &c);

void
ScanWifiTechnology(ODBus::Connection &c);

std::vector<ServiceEntry>
ListServices(ODBus::Connection &c);

std::string
FormatStatus(ODBus::Connection &c);

void
Connect(ODBus::Connection &c, const char *path,
        const char *ssid = nullptr, const char *passphrase = nullptr);

void
Disconnect(ODBus::Connection &c, const char *path);

void
Remove(ODBus::Connection &c, const char *path);

} // namespace CmClient
