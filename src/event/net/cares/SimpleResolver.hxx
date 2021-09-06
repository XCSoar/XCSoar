/*
 * Copyright 2021 Max Kellermann <max.kellermann@gmail.com>
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

#include "Handler.hxx"
#include "net/AllocatedSocketAddress.hxx"
#include "util/Cancellable.hxx"

#include <exception>
#include <forward_list>

namespace Cares {

class Channel;

class SimpleHandler {
public:
	virtual void OnResolverSuccess(std::forward_list<AllocatedSocketAddress> addresses) noexcept = 0;
	virtual void OnResolverError(std::exception_ptr error) noexcept = 0;
};

class SimpleResolver final : Handler {
	std::forward_list<AllocatedSocketAddress> addresses;

	std::exception_ptr error;

	CancellablePointer cancel_ptr;

	SimpleHandler &handler;

	const unsigned port;

public:
	SimpleResolver(SimpleHandler &_handler, unsigned _port=0) noexcept;

	~SimpleResolver() noexcept {
		if (cancel_ptr)
			cancel_ptr.Cancel();
	}

	void Start(Channel &channel, const char *name) noexcept;

private:
	/* virtual methods from Cares::Handler */
	void OnCaresAddress(SocketAddress address) noexcept override {
		addresses.emplace_front(address);
		if (port != 0)
			addresses.front().SetPort(port);
	}

	void OnCaresSuccess() noexcept override {
		cancel_ptr = nullptr;
		addresses.reverse();
		handler.OnResolverSuccess(std::move(addresses));
	}

	void OnCaresError(std::exception_ptr e) noexcept override {
		cancel_ptr = nullptr;
		handler.OnResolverError(std::move(e));
	}
};

} // namespace Cares
