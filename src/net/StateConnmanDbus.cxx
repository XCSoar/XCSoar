// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The XCSoar Project

#include "StateConnmanDbus.hpp"

#include "lib/dbus/Connection.hxx"
#include "lib/dbus/Properties.hxx"
#include "lib/dbus/ReadIter.hxx"

#include <cstring>

#include <dbus/dbus.h>

/* Same bus name and root path as #ConnmanWifi. */
static constexpr const char *kDest = "net.connman";
static constexpr const char *kPath = "/";
static constexpr const char *kIface = "net.connman.Manager";

/* ConnMan session State values (lowercase); unlisted → #nullopt (sysfs). */
static std::optional<NetState>
MapConnmanManagerState(const char *s) noexcept
{
  if (s == nullptr || s[0] == '\0')
    return std::nullopt;

  /* Connected / routable. */
  if (strcmp(s, "online") == 0 || strcmp(s, "ready") == 0)
    return NetState::CONNECTED;

  if (strcmp(s, "idle") == 0 || strcmp(s, "offline") == 0)
    return NetState::DISCONNECTED;

  return std::nullopt;
}

std::optional<NetState>
TryGetNetStateFromConnMan() noexcept
{
  try {
    ODBus::Connection c = ODBus::Connection::GetSystem();

    ODBus::Message reply = ODBus::PropertiesGet(
      c, kDest, kPath, kIface, "State");

    ODBus::ReadMessageIter it{*reply.Get()};
    if (it.GetArgType() != DBUS_TYPE_VARIANT) {
      return std::nullopt;
    }

    ODBus::ReadMessageIter v = it.Recurse();
    if (v.GetArgType() != DBUS_TYPE_STRING) {
      return std::nullopt;
    }

    const char *s = v.GetString();
    return MapConnmanManagerState(s);
  } catch (...) {
    return std::nullopt;
  }
}
