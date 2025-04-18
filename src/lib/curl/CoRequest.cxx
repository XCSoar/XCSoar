// SPDX-License-Identifier: BSD-2-Clause
// Copyright CM4all GmbH
// author: Max Kellermann <mk@cm4all.com>

#include "CoRequest.hxx"
#include "Global.hxx"
#include "util/SpanCast.hxx"

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
CoRequest::OnHeaders(unsigned status, Headers &&headers)
{
	response.status = status;
	response.headers = std::move(headers);
}

void
CoRequest::OnData(std::span<const std::byte> data)
{
	assert(!defer_error.IsPending());

	response.body.append(ToStringView(data));
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
