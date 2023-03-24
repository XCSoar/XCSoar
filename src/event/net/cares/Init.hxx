// SPDX-License-Identifier: BSD-2-Clause
// Copyright CM4all GmbH
// author: Max Kellermann <mk@cm4all.com>

#pragma once

#if defined(__AUGUST__)  // TODO(August2111): Tis isn't correct for make...
#if defined(SSIZE_T)
typedef SSIZE_T ssize_t;
#else // SSIZE_T
typedef long long ssize_t;
#endif
#endif

namespace Cares {

/**
 * Global c-ares initialization.
 */
class Init {
public:
	Init();
	~Init() noexcept;

	Init(const Init &) = delete;
	Init &operator=(const Init &) = delete;
};

} // namespace Cares
