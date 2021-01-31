/*
 * Copyright 2020-2021 CM4all GmbH
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

#include "Handler.hxx"
#include "co/Compat.hxx"
#include "net/AllocatedSocketAddress.hxx"
#include "util/Cancellable.hxx"

#include <exception>
#include <vector>

namespace Cares {

class Channel;

class CoLookup final : Handler {
	std::coroutine_handle<> continuation;

	std::vector<AllocatedSocketAddress> value;

	std::exception_ptr error;

	CancellablePointer cancel_ptr{nullptr};

public:
	CoLookup(Channel &channel, const char *name, int family) noexcept;
	CoLookup(Channel &channel, const char *name) noexcept;

	~CoLookup() noexcept {
		if (cancel_ptr)
			cancel_ptr.Cancel();
	}

	auto operator co_await() noexcept {
		struct Awaitable final {
			CoLookup &lookup;

			bool await_ready() const noexcept {
				return !lookup.cancel_ptr;
			}

			std::coroutine_handle<> await_suspend(std::coroutine_handle<> _continuation) noexcept {
				lookup.continuation = _continuation;
				return std::noop_coroutine();
			}

			decltype(auto) await_resume() {
				if (lookup.error)
					std::rethrow_exception(lookup.error);

				return std::move(lookup.value);
			}
		};

		return Awaitable{*this};
	}

private:
	/* virtual methods from Cares::Handler */
	void OnCaresAddress(SocketAddress address) noexcept override {
		value.emplace_back(address);
	}

	void OnCaresSuccess() noexcept override {
		cancel_ptr = nullptr;

		if (continuation)
			continuation.resume();
	}

	void OnCaresError(std::exception_ptr e) noexcept override {
		cancel_ptr = nullptr;
		error = std::move(e);

		if (continuation)
			continuation.resume();
	}
};

} // namespace Cares
