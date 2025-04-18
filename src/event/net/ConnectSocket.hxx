// SPDX-License-Identifier: BSD-2-Clause
// Copyright CM4all GmbH
// author: Max Kellermann <mk@cm4all.com>

#pragma once

#include "event/SocketEvent.hxx"
#include "event/CoarseTimerEvent.hxx"
#include "util/Cancellable.hxx"

#include <exception>

class UniqueSocketDescriptor;
class SocketAddress;
class AddressInfo;

class ConnectSocketHandler {
public:
	virtual void OnSocketConnectSuccess(UniqueSocketDescriptor fd) noexcept = 0;
	virtual void OnSocketConnectTimeout() noexcept;
	virtual void OnSocketConnectError(std::exception_ptr ep) noexcept = 0;
};

/**
 * A class that connects to a SocketAddress.
 */
class ConnectSocket final : public Cancellable {
	ConnectSocketHandler &handler;

	SocketEvent event;
	CoarseTimerEvent timeout_event;

public:
	ConnectSocket(EventLoop &_event_loop,
		      ConnectSocketHandler &_handler) noexcept;
	~ConnectSocket() noexcept;

	auto &GetEventLoop() const noexcept {
		return event.GetEventLoop();
	}

	bool IsPending() const noexcept {
		return event.IsDefined();
	}

	/**
	 * @param timeout a timeout or a negative value to disable
	 * timeouts
	 */
	bool Connect(const SocketAddress address,
		     Event::Duration timeout) noexcept;

	bool Connect(const AddressInfo &address,
		     Event::Duration timeout) noexcept;

	/**
	 * Wait until the given socket is connected (this method returns
	 * immediately and invokes the #ConnectSocketHandler on completion
	 * or error).
	 */
	void WaitConnected(UniqueSocketDescriptor _fd,
			   Event::Duration timeout=Event::Duration(-1)) noexcept;

	/* virtual methods from Cancellable */
	void Cancel() noexcept override;

private:
	void OnEvent(unsigned events) noexcept;
	void OnTimeout() noexcept;
};
