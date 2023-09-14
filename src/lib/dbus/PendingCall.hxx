// SPDX-License-Identifier: BSD-2-Clause
// Copyright CM4all GmbH
// author: Max Kellermann <mk@cm4all.com>

#pragma once

#include <dbus/dbus.h>

#include <stdexcept>
#include <utility>

namespace ODBus {

class PendingCall {
	DBusPendingCall *pending = nullptr;

	explicit PendingCall(DBusPendingCall *_pending) noexcept
		:pending(_pending) {}

public:
	PendingCall() noexcept = default;

	PendingCall(PendingCall &&src) noexcept
		:pending(std::exchange(src.pending, nullptr)) {}

	~PendingCall() noexcept {
		if (pending != nullptr)
			dbus_pending_call_unref(pending);
	}

	operator bool() const noexcept {
		return pending;
	}

	DBusPendingCall *Get() noexcept {
		return pending;
	}

	PendingCall &operator=(PendingCall &&src) noexcept {
		std::swap(pending, src.pending);
		return *this;
	}

	static PendingCall SendWithReply(DBusConnection *connection,
					 DBusMessage *message,
					 int timeout_milliseconds=-1) {
		DBusPendingCall *pending;
		if (!dbus_connection_send_with_reply(connection,
						     message,
						     &pending,
						     timeout_milliseconds))
			throw std::runtime_error("dbus_connection_send_with_reply() failed");

		if (pending == nullptr)
			throw std::runtime_error("dbus_connection_send_with_reply() failed with pending=NULL");

		return PendingCall(pending);
	}

	bool SetNotify(DBusPendingCallNotifyFunction function,
		       void *user_data,
		       DBusFreeFunction free_user_data=nullptr) noexcept {
		return dbus_pending_call_set_notify(pending,
						    function, user_data,
						    free_user_data);
	}

	void Cancel() noexcept {
		dbus_pending_call_cancel(pending);
	}

	void Block() noexcept {
		dbus_pending_call_block(pending);
	}
};

} /* namespace ODBus */
