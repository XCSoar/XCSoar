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

#include "Request.hxx"
#include "Handler.hxx"
#include "co/Compat.hxx"

#include <exception>

namespace Curl {

struct CoResponse {
	unsigned status = 0;

	std::multimap<std::string, std::string> headers;

	std::string body;
};

/**
 * A CURL HTTP request as a C++20 coroutine.
 */
class CoRequest final : CurlResponseHandler {
	CurlRequest request;

	CoResponse response;
	std::exception_ptr error;

	std::coroutine_handle<> continuation;

	bool ready = false;

public:
	CoRequest(CurlGlobal &global, CurlEasy easy);

	auto operator co_await() noexcept {
		struct Awaitable final {
			CoRequest &request;

			bool await_ready() const noexcept {
				return request.ready;
			}

			std::coroutine_handle<> await_suspend(std::coroutine_handle<> _continuation) const noexcept {
				request.continuation = _continuation;
				return std::noop_coroutine();
			}

			CoResponse await_resume() const {
				if (request.error)
					std::rethrow_exception(std::move(request.error));

				return std::move(request.response);
			}
		};

		return Awaitable{*this};
	}

private:
	/* virtual methods from CurlResponseHandler */
	void OnHeaders(unsigned status,
		       std::multimap<std::string, std::string> &&headers) override;
	void OnData(ConstBuffer<void> data) override;
	void OnEnd() override;
	void OnError(std::exception_ptr e) noexcept override;
};

} // namespace Curl
