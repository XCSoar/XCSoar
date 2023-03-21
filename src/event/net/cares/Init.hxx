// SPDX-License-Identifier: BSD-2-Clause
// Copyright CM4all GmbH
// author: Max Kellermann <mk@cm4all.com>

#pragma once

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
