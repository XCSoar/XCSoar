// SPDX-License-Identifier: BSD-2-Clause
// Copyright CM4all GmbH
// author: Max Kellermann <mk@cm4all.com>

#pragma once

#include <dbus/dbus.h>

namespace ODBus {

class MessageIter {
protected:
	DBusMessageIter iter;

	MessageIter() = default;

public:
	MessageIter(const MessageIter &) = delete;
	MessageIter &operator=(const MessageIter &) = delete;
};

} /* namespace ODBus */
