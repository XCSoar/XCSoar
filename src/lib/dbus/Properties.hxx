// SPDX-License-Identifier: BSD-2-Clause
// Copyright CM4all GmbH
// author: Max Kellermann <max.kellermann@gmail.com>

#pragma once

#include "CallMethodSync.hxx"
#include "Message.hxx"
#include "AppendIter.hxx"
#include "Values.hxx"
#include "Connection.hxx"

#include <utility>

namespace ODBus {

/**
 * D-Bus org.freedesktop.DBus.Properties; also used as the bus
 * destination in some of our call sites.
 */
inline constexpr const char *kDBusPropertiesInterface =
	"org.freedesktop.DBus.Properties";

Message
PropertiesGet(Connection &c, const char *destination, const char *object_path,
	      const char *object_iface, const char *property_name);

/**
 * org.freedesktop.DBus.Properties.Set; the value is wrapped in a D-Bus
 * variant (see #Append(Variant) in #AppendMessageIter).
 */
template<typename T>
void
PropertiesSet(Connection &c, const char *destination, const char *object_path,
	      const char *object_iface, const char *property_name, T &&value)
{
	auto msg = Message::NewMethodCall(
		destination, object_path, kDBusPropertiesInterface, "Set");
	AppendMessageIter{*msg.Get()}.Append(object_iface).Append(property_name).Append(
		Variant(std::forward<T>(value)));
	(void)CallMethodSync(c, msg);
}

} // namespace ODBus
