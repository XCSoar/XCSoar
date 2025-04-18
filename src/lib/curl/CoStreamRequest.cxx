// SPDX-License-Identifier: BSD-2-Clause
// author: Max Kellermann <max.kellermann@gmail.com>

#include "CoStreamRequest.hxx"
#include "io/OutputStream.hxx"

namespace Curl {

void
CoStreamRequest::OnData(std::span<const std::byte> data)
try {
	os.Write(data);
} catch (...) {
	DeferError(std::current_exception());
	throw Pause{};
}

} // namespace Curl
