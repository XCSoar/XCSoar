// SPDX-License-Identifier: BSD-2-Clause
// Copyright CM4all GmbH
// author: Max Kellermann <mk@cm4all.com>

#include "TimeDate.hxx"
#include "Connection.hxx"
#include "Message.hxx"
#include "AppendIter.hxx"
#include "PendingCall.hxx"
#include "ReadIter.hxx"
#include "Error.hxx"

namespace TimeDate {

bool
IsNTPSynchronized(ODBus::Connection &connection)
{
	using namespace ODBus;

	auto msg = Message::NewMethodCall("org.freedesktop.timedate1",
					  "/org/freedesktop/timedate1",
					  "org.freedesktop.DBus.Properties",
					  "Get");

	AppendMessageIter{*msg.Get()}
		.Append("org.freedesktop.timedate1")
		.Append("NTPSynchronized");

	auto pending = PendingCall::SendWithReply(connection, msg.Get());

	dbus_connection_flush(connection);

	pending.Block();

	Message reply = Message::StealReply(*pending.Get());
	reply.CheckThrowError();

	ReadMessageIter iter = ReadMessageIter{*reply.Get()}.Recurse();
	return iter.GetBool();
}

void
SetTime(ODBus::Connection &connection,
	std::chrono::system_clock::time_point t)
{
	using namespace ODBus;

	auto msg = Message::NewMethodCall("org.freedesktop.timedate1",
					  "/org/freedesktop/timedate1",
					  "org.freedesktop.timedate1",
					  "SetTime");

	const int64_t usec = std::chrono::duration_cast<std::chrono::microseconds>(t.time_since_epoch()).count();
	AppendMessageIter{*msg.Get()}.Append(usec).Append(Boolean{false}).Append(Boolean{false});

	auto pending = PendingCall::SendWithReply(connection, msg.Get());

	dbus_connection_flush(connection);

	pending.Block();

	Message reply = Message::StealReply(*pending.Get());
	reply.CheckThrowError();
}

} // namespace TimeDate
