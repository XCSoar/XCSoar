// SPDX-License-Identifier: BSD-2-Clause
// Copyright CM4all GmbH
// author: Max Kellermann <max.kellermann@gmail.com>

#include "Properties.hxx"

namespace ODBus {

Message
PropertiesGet(Connection &c, const char *destination, const char *object_path,
	      const char *object_iface, const char *property_name)
{
	auto msg = Message::NewMethodCall(
		destination, object_path, kDBusPropertiesInterface, "Get");
	AppendMessageIter{*msg.Get()}.Append(object_iface).Append(property_name);
	return CallMethodSync(c, msg);
}

} // namespace ODBus
