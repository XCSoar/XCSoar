// SPDX-License-Identifier: BSD-2-Clause
// Copyright CM4all GmbH
// author: Max Kellermann <mk@cm4all.com>

#include "Channel.hxx"
#include "Handler.hxx"
#include "Error.hxx"
#include "event/SocketEvent.hxx"
#include "net/SocketAddress.hxx"
#include "time/Convert.hxx"
#include "util/Cancellable.hxx"

#include <assert.h>
#include <string.h>

#ifdef _WIN32
#include <winsock2.h>
#else
#include <netinet/in.h>
#include <netdb.h>
#endif

namespace Cares {

class Channel::Socket {
	Channel &channel;
	SocketEvent event;

public:
	Socket(Channel &_channel,
	       SocketDescriptor fd, unsigned events) noexcept
		:channel(_channel),
		 event(channel.GetEventLoop(), BIND_THIS_METHOD(OnSocket),
		       fd) {
		event.Schedule(events);
	}

private:
	void OnSocket(unsigned events) noexcept {
		event.Cancel();
		channel.OnSocket(event.GetSocket(), events);
	}
};

Channel::Channel(EventLoop &event_loop)
	:defer_update_sockets(event_loop, BIND_THIS_METHOD(UpdateSockets)),
	 timeout_event(event_loop, BIND_THIS_METHOD(OnTimeout))
{
	int code = ares_init(&channel);
	if (code != 0)
		throw Error(code, "ares_init() failed");
}

Channel::~Channel() noexcept
{
	ares_destroy(channel);
}

void
Channel::UpdateSockets() noexcept
{
	timeout_event.Cancel();
	sockets.clear();

	ares_socket_t socks[ARES_GETSOCK_MAXNUM];
	const auto s = ares_getsock(channel, socks, ARES_GETSOCK_MAXNUM);

	for (unsigned i = 0; i < ARES_GETSOCK_MAXNUM; ++i) {
		unsigned events = 0;

		if (ARES_GETSOCK_READABLE(s, i))
			events |= SocketEvent::READ;

		if (ARES_GETSOCK_WRITABLE(s, i))
			events |= SocketEvent::WRITE;

		if (events != 0)
			sockets.emplace_front(*this, SocketDescriptor{socks[i]},
					      events);
	}

	struct timeval timeout_buffer;
	const auto *t = ares_timeout(channel, nullptr, &timeout_buffer);
	if (t != nullptr)
		timeout_event.Schedule(ToSteadyClockDuration(*t));
}

void
Channel::OnSocket(SocketDescriptor fd, unsigned events) noexcept
{
	ares_socket_t read_fd = ARES_SOCKET_BAD, write_fd = ARES_SOCKET_BAD;

	if (events & (SocketEvent::READ|SocketEvent::IMPLICIT_FLAGS))
		read_fd = fd.Get();

	if (events & (SocketEvent::WRITE|SocketEvent::IMPLICIT_FLAGS))
		write_fd = fd.Get();

	ares_process_fd(channel, read_fd, write_fd);

	ScheduleUpdateSockets();
}

void
Channel::OnTimeout() noexcept
{
	sockets.clear();

	ares_process_fd(channel, ARES_SOCKET_BAD, ARES_SOCKET_BAD);

	ScheduleUpdateSockets();
}

template<typename F>
static void
AsSocketAddress(const struct hostent &he, const void *src, F &&f)
{
	switch (he.h_addrtype) {
	case AF_INET:
		{
			struct sockaddr_in sin{};
			memcpy(&sin.sin_addr, src, he.h_length);
			sin.sin_family = AF_INET;

			f(SocketAddress((const struct sockaddr *)&sin,
					sizeof(sin)));
		}
		break;

	case AF_INET6:
		{
			struct sockaddr_in6 sin{};
			memcpy(&sin.sin6_addr, src, he.h_length);
			sin.sin6_family = AF_INET6;

			f(SocketAddress((const struct sockaddr *)&sin,
					sizeof(sin)));
		}
		break;

	default:
		throw std::runtime_error("Unsupported address type");
	}
}

class Channel::Request final : Cancellable {
	Handler *handler;

	unsigned pending;

	bool success = false;

public:
	Request(Handler &_handler, unsigned _pending,
		CancellablePointer &cancel_ptr)
		:handler(&_handler), pending(_pending)
	{
		assert(pending > 0);

		cancel_ptr = *this;
	}

	void Start(ares_channel _channel,
		   const char *name, int family) noexcept {
		assert(handler != nullptr);

		ares_gethostbyname(_channel, name, family,
				   HostCallback, this);
	}

private:
	void Cancel() noexcept override {
		assert(handler != nullptr);
		assert(pending > 0);

		/* c-ares doesn't support cancellation, so we emulate
		   it here */
		handler = nullptr;
	}

	void HostCallback(int status, struct hostent *he) noexcept;

	static void HostCallback(void *arg, int status, int,
				 struct hostent *hostent) noexcept {
		auto &request = *(Request *)arg;
		request.HostCallback(status, hostent);
	}
};

inline void
Channel::Request::HostCallback(int status, struct hostent *he) noexcept
{
	assert(pending > 0);

	if (handler == nullptr)
		/* cancelled */
		return;

	--pending;

	try {
		if (status != ARES_SUCCESS)
			throw Error(status, "ares_gethostbyname() failed");
		else if (he != nullptr) {
			success = true;

			for (auto i = he->h_addr_list; *i != nullptr; ++i)
				AsSocketAddress(*he, *i, [&_handler = *handler](SocketAddress address){
					_handler.OnCaresAddress(address);
				});

			if (pending == 0)
				handler->OnCaresSuccess();
		} else
			throw std::runtime_error("ares_gethostbyname() failed");
	} catch (...) {
		if (pending == 0) {
			if (success)
				handler->OnCaresSuccess();
			else
				handler->OnCaresError(std::current_exception());
		}
	}

	if (pending == 0)
		delete this;
}

void
Channel::Lookup(const char *name, int family, Handler &handler,
		CancellablePointer &cancel_ptr) noexcept
{
	auto *request = new Request(handler, 1, cancel_ptr);
	request->Start(channel, name, family);
	UpdateSockets();
}

void
Channel::Lookup(const char *name, Handler &handler,
		CancellablePointer &cancel_ptr) noexcept
{
	auto *request = new Request(handler, 2, cancel_ptr);
	request->Start(channel, name, AF_INET6);
	request->Start(channel, name, AF_INET);
	UpdateSockets();
}

} // namespace Cares
