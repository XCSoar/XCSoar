// SPDX-License-Identifier: BSD-2-Clause
// Copyright CM4all GmbH
// author: Max Kellermann <max.kellermann@gmail.com>

#pragma once

#include "Connection.hxx"
#include "Message.hxx"

namespace ODBus {

/**
 * Send a method call, block for the reply, and optionally validate it with
 * Message::CheckThrowError().  Centralizes send + flush + pending + steal.
 *
 * @param check_error if true (default), throw when the reply is a D-Bus
 * error; if false, return the message so the caller can inspect
 * Message::IsError() first (see Systemd GetUnit).
 */
Message
CallMethodSync(Connection &c, DBusMessage *request, bool check_error = true);

inline Message
CallMethodSync(Connection &c, Message &request, bool check_error = true)
{
	return CallMethodSync(c, request.Get(), check_error);
}

} // namespace ODBus
