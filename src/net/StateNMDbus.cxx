// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The XCSoar Project

#include "StateNMDbus.hpp"

#include "lib/dbus/Connection.hxx"
#include "lib/dbus/Properties.hxx"
#include "lib/dbus/ReadIter.hxx"

#include <cstdint>
#include <optional>

#include <dbus/dbus.h>

/* Values match NetworkManager NMConnectivityState; keep in sync for mapping. */
static constexpr std::uint32_t kConnectivityUnknown = 0U;
static constexpr std::uint32_t kConnectivityNone = 1U;
static constexpr std::uint32_t kConnectivityPortal = 2U;
static constexpr std::uint32_t kConnectivityLimited = 3U;
static constexpr std::uint32_t kConnectivityFull = 4U;

static constexpr const char *kNmDest = "org.freedesktop.NetworkManager";
static constexpr const char *kNmPath = "/org/freedesktop/NetworkManager";
static constexpr const char *kNmIface = "org.freedesktop.NetworkManager";

static std::optional<NetState>
MapConnectivityToNetState(std::uint32_t c) noexcept
{
  switch (c) {
  case kConnectivityFull:
  case kConnectivityLimited:
  case kConnectivityPortal:
    return NetState::CONNECTED;

  case kConnectivityNone:
    return NetState::DISCONNECTED;

  case kConnectivityUnknown:
  default:
    return std::nullopt;
  }
}

std::optional<NetState>
TryGetNetStateFromNetworkManager() noexcept
{
  try {
    ODBus::Connection c = ODBus::Connection::GetSystem();

    ODBus::Message reply = ODBus::PropertiesGet(
      c, kNmDest, kNmPath, kNmIface, "Connectivity");

    ODBus::ReadMessageIter it{*reply.Get()};
    if (it.GetArgType() != DBUS_TYPE_VARIANT) {
      return std::nullopt;
    }

    ODBus::ReadMessageIter v = it.Recurse();
    if (v.GetArgType() != DBUS_TYPE_UINT32) {
      return std::nullopt;
    }

    dbus_uint32_t u = 0U;
    v.GetBasic(&u);
    return MapConnectivityToNetState(u);
  } catch (...) {
    return std::nullopt;
  }
}
