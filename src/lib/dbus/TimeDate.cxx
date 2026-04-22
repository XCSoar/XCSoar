// SPDX-License-Identifier: BSD-2-Clause
// Copyright CM4all GmbH
// author: Max Kellermann <mk@cm4all.com>

#include "TimeDate.hxx"
#include "CallMethodSync.hxx"
#include "Connection.hxx"
#include "Message.hxx"
#include "Properties.hxx"
#include "AppendIter.hxx"
#include "ReadIter.hxx"
#include "Error.hxx"

namespace TimeDate {

bool
IsNTPSynchronized(ODBus::Connection &connection)
{
	using namespace ODBus;

	Message reply = PropertiesGet(
		connection, "org.freedesktop.timedate1", "/org/freedesktop/timedate1",
		"org.freedesktop.timedate1", "NTPSynchronized");

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

	(void)CallMethodSync(connection, msg);
}

} // namespace TimeDate
