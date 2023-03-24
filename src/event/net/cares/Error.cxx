// SPDX-License-Identifier: BSD-2-Clause
// Copyright CM4all GmbH
// author: Max Kellermann <mk@cm4all.com>

#include "Error.hxx"
#include "util/RuntimeError.hxx"

#include <ares.h>

namespace Cares {

Error::Error(int _code, const char *msg) noexcept
	:std::runtime_error(FormatRuntimeError("%s: %s",
					       msg, ares_strerror(_code))),
	 code(_code) {}

} // namespace Cares
