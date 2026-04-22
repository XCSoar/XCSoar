// SPDX-License-Identifier: BSD-2-Clause
// Copyright CM4all GmbH
// author: Max Kellermann <max.kellermann@gmail.com>

#include "CallMethodSync.hxx"
#include "PendingCall.hxx"

namespace ODBus {

Message
CallMethodSync(Connection &c, DBusMessage *request, bool check_error)
{
	auto pending = PendingCall::SendWithReply(c, request);
	dbus_connection_flush(c);
	pending.Block();
	Message reply = Message::StealReply(*pending.Get());
	if (check_error)
		reply.CheckThrowError();
	return reply;
}

} // namespace ODBus
