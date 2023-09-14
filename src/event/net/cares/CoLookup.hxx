// SPDX-License-Identifier: BSD-2-Clause
// Copyright CM4all GmbH
// author: Max Kellermann <mk@cm4all.com>

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
