// SPDX-License-Identifier: BSD-2-Clause
// author: Max Kellermann <max.kellermann@gmail.com>

#pragma once

#include "Handler.hxx"
#include "thread/AsyncWaiter.hxx"

class OutputStream;

/**
 * A #CurlResponseHandler implementation which writes to an
 * #OutputStream.
 */
class OutputStreamCurlResponseHandler : public CurlResponseHandler {
	OutputStream &os;

	AsyncWaiter waiter;

public:
	explicit OutputStreamCurlResponseHandler(OutputStream &_os) noexcept
		:os(_os) {}

	void Cancel() noexcept {
		waiter.SetDone();
	}

	void Wait() {
		waiter.Wait();
	}

	/* virtual methods from class CurlResponseHandler */
	void OnData(std::span<const std::byte> data) override;

	void OnEnd() final {
		waiter.SetDone();
	}

	void OnError(std::exception_ptr e) noexcept final {
		waiter.SetError(std::move(e));
	}
};
