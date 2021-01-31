/*
 * Copyright 2017-2021 CM4all GmbH
 * All rights reserved.
 *
 * author: Max Kellermann <mk@cm4all.com>
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 *
 * - Redistributions of source code must retain the above copyright
 * notice, this list of conditions and the following disclaimer.
 *
 * - Redistributions in binary form must reproduce the above copyright
 * notice, this list of conditions and the following disclaimer in the
 * documentation and/or other materials provided with the
 * distribution.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
 * ``AS IS'' AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
 * LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS
 * FOR A PARTICULAR PURPOSE ARE DISCLAIMED.  IN NO EVENT SHALL THE
 * FOUNDATION OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT,
 * INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES
 * (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR
 * SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT,
 * STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
 * ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED
 * OF THE POSSIBILITY OF SUCH DAMAGE.
 */

#pragma once

#include "Init.hxx"
#include "event/DeferEvent.hxx"
#include "event/TimerEvent.hxx"

#include <ares.h>

#include <forward_list>

class CancellablePointer;
class SocketDescriptor;

namespace Cares {

class Handler;

/**
 * C++ wrapper for #ares_channel with #EventLoop integration.
 */
class Channel {
	Init init;

	ares_channel channel;

	DeferEvent defer_process;
	TimerEvent timeout_event;

	class Socket;

	std::forward_list<Socket> sockets;
	fd_set read_ready, write_ready;

	class Request;

public:
	explicit Channel(EventLoop &event_loop);
	~Channel() noexcept;

	EventLoop &GetEventLoop() const noexcept {
		return defer_process.GetEventLoop();
	}

	/**
	 * Look up a host name and call a #Handler method upon
	 * completion.
	 */
	void Lookup(const char *name, int family, Handler &handler,
		    CancellablePointer &cancel_ptr) noexcept;

	/**
	 * This overload submits two queries: AF_INET and AF_INET6 and
	 * returns both results to the handler.
	 */
	void Lookup(const char *name, Handler &handler,
		    CancellablePointer &cancel_ptr) noexcept;

private:
	void UpdateSockets() noexcept;
	void DeferredProcess() noexcept;

	void ScheduleProcess() noexcept {
		defer_process.Schedule();
	}

	void OnSocket(SocketDescriptor fd, unsigned events) noexcept;
	void OnTimeout() noexcept;
};

} // namespace Cares
