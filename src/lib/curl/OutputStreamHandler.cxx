// SPDX-License-Identifier: BSD-2-Clause
// author: Max Kellermann <max.kellermann@gmail.com>

#include "OutputStreamHandler.hxx"
#include "io/OutputStream.hxx"

void
OutputStreamCurlResponseHandler::OnData(std::span<const std::byte> data)
{
	try {
		os.Write(data);
	} catch (...) {
		waiter.SetError(std::current_exception());
		throw Pause{};
	}
}
