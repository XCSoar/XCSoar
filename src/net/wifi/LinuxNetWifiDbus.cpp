// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The XCSoar Project

#include "LinuxNetWifiDbus.hpp"
#include "lib/dbus/AppendIter.hxx"
#include "lib/dbus/CallMethodSync.hxx"
#include "lib/dbus/Message.hxx"
#include "lib/dbus/Properties.hxx"
#include "lib/dbus/ReadIter.hxx"
#include "lib/dbus/Values.hxx"

#include <stdexcept>

/* Well-known name of the D-Bus peer; org.freedesktop.DBus.Properties is the
   *method interface*, not a bus destination. */
static constexpr const char *kNetworkManagerBus = "org.freedesktop.NetworkManager";

bool
LinuxNetWifi::NameHasOwner(ODBus::Connection &c, const char *name)
{
	using namespace ODBus;

	auto msg = Message::NewMethodCall("org.freedesktop.DBus",
					  "/org/freedesktop/DBus",
					  "org.freedesktop.DBus",
					  "NameHasOwner");
	AppendMessageIter{*msg.Get()}.Append(name);
	Message reply = CallMethodSync(c, msg);

	ReadMessageIter iter{*reply.Get()};
	if (iter.GetArgType() == DBUS_TYPE_BOOLEAN) {
		return iter.GetBool() != 0;
	}
	throw std::runtime_error("NameHasOwner: unexpected return type");
}

void
LinuxNetWifi::DbusGetProperty(ODBus::Connection &c,
                              const char *path, const char *obj_iface,
                              const char *name,
                              std::string *str_out, std::uint32_t *u32_out,
                              bool *bool_out)
{
	using namespace ODBus;

	if (str_out)
		str_out->clear();
	if (u32_out)
		*u32_out = 0;
	if (bool_out)
		*bool_out = false;

	Message reply = PropertiesGet(
		c, kNetworkManagerBus, path, obj_iface, name);

	ReadMessageIter it{*reply.Get()};
	if (it.GetArgType() != DBUS_TYPE_VARIANT) {
		if (str_out)
			*str_out = {};
		return;
	}

	ReadMessageIter inner = it.Recurse();
	const int t = inner.GetArgType();
	if (str_out != nullptr &&
	    (t == DBUS_TYPE_STRING || t == DBUS_TYPE_OBJECT_PATH)) {
		const char *p = inner.GetString();
		*str_out = p != nullptr ? p : "";
	}
	if (u32_out != nullptr && t == DBUS_TYPE_UINT32) {
		dbus_uint32_t u = 0;
		inner.GetBasic(&u);
		*u32_out = u;
	}
	if (bool_out != nullptr && t == DBUS_TYPE_BOOLEAN)
		*bool_out = inner.GetBool() != 0;
}

void
LinuxNetWifi::DbusGetByteStringProperty(ODBus::Connection &c,
                                        const char *path, const char *obj_iface,
                                        const char *name, std::string &out)
{
	using namespace ODBus;
	out.clear();

	Message reply = PropertiesGet(
		c, kNetworkManagerBus, path, obj_iface, name);

	ReadMessageIter it{*reply.Get()};
	if (it.GetArgType() != DBUS_TYPE_VARIANT)
		return;
	ReadMessageIter v = it.Recurse();
	const int inner = v.GetArgType();
	/* Ssid: usually "ay"; some stacks use plain UTF-8 "s" or (rarely) "as". */
	if (inner == DBUS_TYPE_STRING) {
		const char *p = v.GetString();
		if (p != nullptr)
			out = p;
		return;
	}
	if (inner != DBUS_TYPE_ARRAY)
		return;
	if (v.GetArrayElementType() == DBUS_TYPE_BYTE) {
		/* Do not use dbus_message_iter_get_fixed_array(): it is strict and
		   can assert on some messages; walk "ay" elements instead. */
		for (ReadMessageIter b = v.Recurse();
		     b.GetArgType() == DBUS_TYPE_BYTE; b.Next()) {
			unsigned char ch = 0;
			b.GetBasic(&ch);
			out.push_back(static_cast<char>(ch));
		}
	} else if (v.GetArrayElementType() == DBUS_TYPE_STRING) {
		/* e.g. array of zero or one ESSID as string (non-fixed array). */
		for (ReadMessageIter e = v.Recurse();
		     e.GetArgType() == DBUS_TYPE_STRING; e.Next()) {
			const char *p = e.GetString();
			if (p != nullptr) {
				out = p;
				return;
			}
		}
	}
}

void
LinuxNetWifi::DbusSetPropertyBoolean(ODBus::Connection &c, const char *path,
                                     const char *object_iface, const char *name,
                                     bool value)
{
	using namespace ODBus;

	PropertiesSet(c, kNetworkManagerBus, path, object_iface, name, Boolean{value});
}
