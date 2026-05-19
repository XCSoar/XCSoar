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

#include <exception>

namespace {

static constexpr const char *NM_NAME = "org.freedesktop.NetworkManager";
static constexpr const char *NM_PATH = "/org/freedesktop/NetworkManager";
static constexpr const char *CONNMAN_NAME = "net.connman";

static ODBus::Connection
GetSystemConnection()
{
  try {
    auto c = ODBus::Connection::GetSystem();
    if (!c)
      throw WifiError::Exception{WifiError::Code::NoDbusConnection};

    return c;
  } catch (const WifiError::Exception &) {
    throw;
  } catch (...) {
    std::throw_with_nested(WifiError::Exception{WifiError::Code::NoDbusConnection});
  }
}

} // namespace

LinuxWifiBackendKind
QueryLinuxWifiBackendKind()
{
  auto c = GetSystemConnection();

  if (LinuxNetWifi::NameHasOwner(c, NM_NAME) &&
      !NmClient::FindWifiDevice(c).empty())
    return LinuxWifiBackendKind::NetworkManager;

  if (LinuxNetWifi::NameHasOwner(c, CONNMAN_NAME) &&
      CmClient::HasWifiTechnology(c))
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
  auto c = GetSystemConnection();

  switch (backend_kind) {
  case LinuxWifiBackendKind::NetworkManager: {
    if (!LinuxNetWifi::NameHasOwner(c, NM_NAME))
      throw WifiError::Exception{WifiError::Code::NetworkManagerUnavailable};

    bool enabled = true;
    LinuxNetWifi::DbusGetProperty(c, NM_PATH, NM_NAME, "WirelessEnabled",
                                  nullptr, nullptr, &enabled);
    return enabled;
  }

  case LinuxWifiBackendKind::ConnMan:
    if (!LinuxNetWifi::NameHasOwner(c, CONNMAN_NAME))
      throw WifiError::Exception{WifiError::Code::ConnmanUnavailable};

    if (!CmClient::HasWifiTechnology(c))
      throw WifiError::Exception{WifiError::Code::NoInterface};

    return CmClient::IsWifiTechnologyPowered(c);

  case LinuxWifiBackendKind::None:
    throw WifiError::Exception{WifiError::Code::NoBackendAvailable};
  }

  throw WifiError::Exception{WifiError::Code::NoBackendAvailable};
}

void
SetLinuxWifiRadioEnabled(LinuxWifiBackendKind backend_kind, bool enabled)
{
  auto c = GetSystemConnection();

  switch (backend_kind) {
  case LinuxWifiBackendKind::NetworkManager:
    if (!LinuxNetWifi::NameHasOwner(c, NM_NAME))
      throw WifiError::Exception{WifiError::Code::NetworkManagerUnavailable};

    NmClient::SetWirelessEnabled(c, enabled);
    return;

  case LinuxWifiBackendKind::ConnMan:
    if (!LinuxNetWifi::NameHasOwner(c, CONNMAN_NAME))
      throw WifiError::Exception{WifiError::Code::ConnmanUnavailable};

    if (!CmClient::HasWifiTechnology(c))
      throw WifiError::Exception{WifiError::Code::NoInterface};

    CmClient::SetWifiTechnologyPowered(c, enabled);
    return;

  case LinuxWifiBackendKind::None:
    throw WifiError::Exception{WifiError::Code::NoBackendAvailable};
  }

  throw WifiError::Exception{WifiError::Code::NoBackendAvailable};
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