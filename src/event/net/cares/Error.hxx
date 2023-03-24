// SPDX-License-Identifier: BSD-2-Clause
// Copyright CM4all GmbH
// author: Max Kellermann <mk@cm4all.com>

#pragma once

#include <stdexcept>

#if defined(__AUGUST__)  // TODO(August2111): Tis isn't correct for make...
#ifdef SSIZE_T
typedef SSIZE_T ssize_t;
#else   // SSIZE_T
typedef long long ssize_t;
#endif
#endif

namespace Cares {

class Error : public std::runtime_error {
	int code;

public:
	Error(int _code, const char *msg) noexcept;

	int GetCode() const noexcept {
		return code;
	}
};

} // namespace Cares
