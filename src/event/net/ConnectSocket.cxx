/*
 * Copyright 2007-2021 CM4all GmbH
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

#include "ConnectSocket.hxx"
#include "net/UniqueSocketDescriptor.hxx"
#include "net/SocketAddress.hxx"
#include "net/AddressInfo.hxx"
#include "net/SocketError.hxx"

#include <cassert>
#include <stdexcept>

void
ConnectSocketHandler::OnSocketConnectTimeout() noexcept
{
	/* default implementation falls back to OnSocketConnectError() */
	OnSocketConnectError(std::make_exception_ptr(std::runtime_error("Connect timeout")));
}

ConnectSocket::ConnectSocket(EventLoop &_event_loop,
			     ConnectSocketHandler &_handler) noexcept
	:handler(_handler),
	 event(_event_loop, BIND_THIS_METHOD(OnEvent)),
	 timeout_event(_event_loop, BIND_THIS_METHOD(OnTimeout))
{
}

ConnectSocket::~ConnectSocket() noexcept
{
	event.Close();
}

void
ConnectSocket::Cancel() noexcept
{
	assert(IsPending());

	timeout_event.Cancel();
	event.Close();
}

static UniqueSocketDescriptor
Connect(const SocketAddress address)
{
	UniqueSocketDescriptor fd;
	if (!fd.CreateNonBlock(address.GetFamily(), SOCK_STREAM, 0))
		throw MakeSocketError("Failed to create socket");

	if (!fd.Connect(address)) {
		const auto e = GetSocketError();
		if (!IsSocketErrorConnectWouldBlock(e))
			throw MakeSocketError(e, "Failed to connect");
	}

	return fd;
}

bool
ConnectSocket::Connect(const SocketAddress address,
		       Event::Duration timeout) noexcept
{
	assert(!event.IsDefined());

	try {
		WaitConnected(::Connect(address), timeout);
		return true;
	} catch (...) {
		handler.OnSocketConnectError(std::current_exception());
		return false;
	}
}

static UniqueSocketDescriptor
Connect(const AddressInfo &address)
{
	UniqueSocketDescriptor fd;
	if (!fd.CreateNonBlock(address.GetFamily(), address.GetType(),
			       address.GetProtocol()))
		throw MakeSocketError("Failed to create socket");

	if (!fd.Connect(address)) {
		const auto e = GetSocketError();
		if (!IsSocketErrorConnectWouldBlock(e))
			throw MakeSocketError(e, "Failed to connect");
	}

	return fd;
}

bool
ConnectSocket::Connect(const AddressInfo &address,
		       Event::Duration timeout) noexcept
{
	assert(!event.IsDefined());

	try {
		WaitConnected(::Connect(address), timeout);
		return true;
	} catch (...) {
		handler.OnSocketConnectError(std::current_exception());
		return false;
	}
}

void
ConnectSocket::WaitConnected(UniqueSocketDescriptor _fd,
			     Event::Duration timeout) noexcept
{
	assert(!event.IsDefined());

	event.Open(_fd.Release());
	event.ScheduleWrite();

	if (timeout >= Event::Duration{})
		timeout_event.Schedule(timeout);
}

void
ConnectSocket::OnEvent(unsigned events) noexcept
{
	timeout_event.Cancel();

	if (SocketEvent::ERROR == 0 || events & SocketEvent::ERROR) {
		int s_err = event.GetSocket().GetError();
		if (s_err != 0) {
			event.Close();
			handler.OnSocketConnectError(std::make_exception_ptr(MakeSocketError(s_err, "Failed to connect")));
			return;
		}
	}

	handler.OnSocketConnectSuccess(UniqueSocketDescriptor(event.ReleaseSocket()));
}

void
ConnectSocket::OnTimeout() noexcept
{
	event.Close();

	handler.OnSocketConnectTimeout();
}
