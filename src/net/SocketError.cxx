// SPDX-License-Identifier: BSD-2-Clause
// author: Max Kellermann <max.kellermann@gmail.com>

#include "SocketError.hxx"

#include <iterator>

#include <string.h>

#ifdef _WIN32

SocketErrorMessage::SocketErrorMessage(socket_error_t code) noexcept
{
	auto *buffer = msg;

	DWORD nbytes = FormatMessage(FORMAT_MESSAGE_FROM_SYSTEM |
				     FORMAT_MESSAGE_IGNORE_INSERTS |
				     FORMAT_MESSAGE_MAX_WIDTH_MASK,
				     nullptr, code, 0,
				     buffer, msg_size, nullptr);
	if (nbytes == 0) {
		strcpy(msg, "Unknown error");
		return;
	}
}

#else

SocketErrorMessage::SocketErrorMessage(socket_error_t code) noexcept
	:msg(strerror(code)) {}

#endif
