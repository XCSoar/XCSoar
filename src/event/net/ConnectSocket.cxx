// SPDX-License-Identifier: BSD-2-Clause
// Copyright CM4all GmbH
// author: Max Kellermann <mk@cm4all.com>

#include "ConnectSocket.hxx"
#include "net/UniqueSocketDescriptor.hxx"
#include "net/SocketAddress.hxx"
#include "net/AddressInfo.hxx"
#include "net/SocketError.hxx"
#include "net/TimeoutError.hxx"

#include <cassert>
#include <stdexcept>

void
ConnectSocketHandler::OnSocketConnectTimeout() noexcept
{
	/* default implementation falls back to OnSocketConnectError() */
	OnSocketConnectError(std::make_exception_ptr(TimeoutError{"Connect timeout"}));
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
