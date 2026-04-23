// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The XCSoar Project

#include "LinuxWifiBackend.hpp"

#include "ConnmanClient.hpp"
#include "ConnmanWifiBackend.hpp"
#include "LinuxNetWifiDbus.hpp"
#include "NetworkManagerClient.hpp"
#include "NetworkManagerWifiBackend.hpp"
#include "WifiError.hpp"
#include "lib/dbus/Connection.hxx"
#include "util/Compiler.h"

namespace {

static constexpr const char *NM_NAME = "org.freedesktop.NetworkManager";
static constexpr const char *NM_PATH = "/org/freedesktop/NetworkManager";
static constexpr const char *CONNMAN_NAME = "net.connman";

} // namespace

LinuxWifiBackendKind
QueryLinuxWifiBackendKind()
{
  auto c = ODBus::Connection::GetSystem();
  if (!c)
    return LinuxWifiBackendKind::None;

  if (LinuxNetWifi::NameHasOwner(c, NM_NAME) &&
      !NmClient::FindWifiDevice(c).empty())
    return LinuxWifiBackendKind::NetworkManager;

  if (LinuxNetWifi::NameHasOwner(c, CONNMAN_NAME))
    return LinuxWifiBackendKind::ConnMan;

  return LinuxWifiBackendKind::None;
}

bool
HasLinuxWifiRadioToggle(LinuxWifiBackendKind backend_kind) noexcept
{
  return backend_kind == LinuxWifiBackendKind::NetworkManager ||
    backend_kind == LinuxWifiBackendKind::ConnMan;
}

bool
GetLinuxWifiRadioEnabled(LinuxWifiBackendKind backend_kind)
{
  auto c = ODBus::Connection::GetSystem();
  if (!c)
    throw std::runtime_error{FormatWifiErrorForUser("No D-Bus connection")};

  switch (backend_kind) {
  case LinuxWifiBackendKind::NetworkManager: {
    bool enabled = true;
    LinuxNetWifi::DbusGetProperty(c, NM_PATH, NM_NAME, "WirelessEnabled",
                                  nullptr, nullptr, &enabled);
    return enabled;
  }

  case LinuxWifiBackendKind::ConnMan:
    return CmClient::IsWifiTechnologyPowered(c);

  case LinuxWifiBackendKind::None:
    throw std::runtime_error{
      FormatWifiErrorForUser("No Wi-Fi backend available")};
  }

  throw std::runtime_error{FormatWifiErrorForUser("No Wi-Fi backend available")};
}

void
SetLinuxWifiRadioEnabled(LinuxWifiBackendKind backend_kind, bool enabled)
{
  auto c = ODBus::Connection::GetSystem();
  if (!c)
    throw std::runtime_error{FormatWifiErrorForUser("No D-Bus connection")};

  switch (backend_kind) {
  case LinuxWifiBackendKind::NetworkManager:
    NmClient::SetWirelessEnabled(c, enabled);
    return;

  case LinuxWifiBackendKind::ConnMan:
    CmClient::SetWifiTechnologyPowered(c, enabled);
    return;

  case LinuxWifiBackendKind::None:
    throw std::runtime_error{
      FormatWifiErrorForUser("No Wi-Fi backend available")};
  }

  throw std::runtime_error{FormatWifiErrorForUser("No Wi-Fi backend available")};
}

UniqueWifiBackend
CreateLinuxWifiBackend(LinuxWifiBackendKind backend_kind)
{
  switch (backend_kind) {
  case LinuxWifiBackendKind::NetworkManager:
    return std::make_unique<NetworkManagerWifiBackend>();

  case LinuxWifiBackendKind::ConnMan:
    return std::make_unique<ConnmanWifiBackend>();

  case LinuxWifiBackendKind::None:
    return {};
  }

  gcc_unreachable();
}

UniqueWifiBackend
CreateLinuxWifiBackend()
{
  return CreateLinuxWifiBackend(QueryLinuxWifiBackendKind());
}