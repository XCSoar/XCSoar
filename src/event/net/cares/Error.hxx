// SPDX-License-Identifier: BSD-2-Clause
// Copyright CM4all GmbH
// author: Max Kellermann <mk@cm4all.com>

#pragma once

#include <stdexcept>

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
