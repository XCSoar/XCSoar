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

#include "CoRequest.hxx"
#include "Global.hxx"

namespace Curl {

CoRequest::CoRequest(CurlGlobal &global, CurlEasy easy)
	:request(global, std::move(easy), *this),
	 defer_error(global.GetEventLoop(), BIND_THIS_METHOD(OnDeferredError))
{
	request.Start();
}

void
CoRequest::DeferError(std::exception_ptr _error) noexcept
{
	assert(_error);
	assert(!error);
	assert(!ready);
	assert(!defer_error.IsPending());

	error = std::move(_error);
	ready = true;

	if (continuation)
		defer_error.Schedule();
}

inline void
CoRequest::OnDeferredError() noexcept
{
	assert(error);
	assert(ready);
	assert(continuation);

	continuation.resume();
}

void
CoRequest::OnHeaders(unsigned status,
		     std::multimap<std::string, std::string> &&headers)
{
	response.status = status;
	response.headers = std::move(headers);
}

void
CoRequest::OnData(ConstBuffer<void> data)
{
	assert(!defer_error.IsPending());

	response.body.append((const char *)data.data,
			     data.size);
}

void
CoRequest::OnEnd()
{
	assert(!defer_error.IsPending());

	ready = true;

	defer_error.Cancel();

	if (continuation)
		continuation.resume();
}

void
CoRequest::OnError(std::exception_ptr e) noexcept
{
	assert(!defer_error.IsPending());

	error = std::move(e);
	ready = true;

	if (continuation)
		continuation.resume();
}

} // namespace Curl
