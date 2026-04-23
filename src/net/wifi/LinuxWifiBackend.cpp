// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The XCSoar Project

#include "LinuxWifiBackend.hpp"

#include "ConnmanWifiBackend.hpp"
#include "LinuxNetWifiDbus.hpp"
#include "NetworkManagerClient.hpp"
#include "NetworkManagerWifiBackend.hpp"
#include "lib/dbus/Connection.hxx"

namespace {

static constexpr const char *kNmName = "org.freedesktop.NetworkManager";
static constexpr const char *kConnmanName = "net.connman";

} // namespace

UniqueWifiBackend
CreateLinuxWifiBackend()
{
  try {
    auto c = ODBus::Connection::GetSystem();
    if (!c)
      return {};

    if (LinuxNetWifi::NameHasOwner(c, kNmName) &&
        !NmClient::FindWifiDevice(c).empty())
      return std::make_unique<NetworkManagerWifiBackend>();

    if (LinuxNetWifi::NameHasOwner(c, kConnmanName))
      return std::make_unique<ConnmanWifiBackend>();
  } catch (...) {
  }

  return {};
}