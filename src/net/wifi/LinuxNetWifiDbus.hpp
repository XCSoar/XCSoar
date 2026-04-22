// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The XCSoar Project

#pragma once

#include "lib/dbus/Connection.hxx"
#include "util/StringAPI.hxx"

#include <cstdint>
#include <string>

namespace LinuxNetWifi {

/**
 * D-Bus object paths that are "unset" are often "" or the root path "/".
 */
[[gnu::pure]]
inline bool
DbusObjectPathIsEmpty(const std::string &p) noexcept
{
	return p.empty() || StringIsEqual(p.c_str(), "/");
}


/**
 * @return whether `name` is owned (service running) on the system bus.
 * @throws on D-Bus failure
 */
bool
NameHasOwner(ODBus::Connection &c, const char *name);

/**
 * org.freedesktop.DBus.Properties.Get to NetworkManager objects; decodes one
 * variant. At most one of the out pointers should be used per call, matching
 * the property type.
 */
void
DbusGetProperty(ODBus::Connection &c,
                const char *path, const char *obj_iface,
                const char *name,
                std::string *str_out, std::uint32_t *u32_out, bool *bool_out);

/**
 * Properties.Get for a byte-array property (e.g. WiFi Ssid) returned as
 * a byte string (not UTF-8 normalised; typically ASCII/UTF-8).
 */
void
DbusGetByteStringProperty(ODBus::Connection &c,
                            const char *path, const char *obj_iface,
                            const char *name, std::string &out);

void
DbusSetPropertyBoolean(ODBus::Connection &c, const char *path,
                      const char *object_iface, const char *name,
                      bool value);

} // namespace LinuxNetWifi
