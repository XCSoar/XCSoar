// SPDX-License-Identifier: BSD-2-Clause
// author: Max Kellermann <max.kellermann@gmail.com>

#pragma once

#include "CoRequest.hxx"

class OutputStream;

namespace Curl {

/**
 * A variant of #CoRequest which redirects the response body to an
 * #OutputStream.  The caller owns the #OutputStream, and errors
 * thrown by OutputStream::Write() will be rethrown to the awaiter.
 */
class CoStreamRequest : public CoRequest {
	OutputStream &os;

public:
	CoStreamRequest(CurlGlobal &global, CurlEasy &&easy, OutputStream &_os)
		:CoRequest(global, std::move(easy)), os(_os) {}

private:
	/* virtual methods from CurlResponseHandler */
	void OnData(std::span<const std::byte> data) override;
};

} // namespace Curl
