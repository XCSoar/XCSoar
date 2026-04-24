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
  bool needs_key{false};
  int strength{0};
};

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

void
Disconnect(ODBus::Connection &c, const char *path);

} // namespace CmClient
