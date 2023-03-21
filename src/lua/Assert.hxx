// SPDX-License-Identifier: BSD-2-Clause
// author: Max Kellermann <max.kellermann@gmail.com>

#pragma once

extern "C" {
#include <lua.h>
}

#ifndef NDEBUG
#include <cassert>
#endif

namespace Lua {

/**
 * A scope class which verifies Lua's stack top in the destructor via
 * assert().  In NDEBUG mode, this is a no-op and gets optimized away.
 */
class ScopeCheckStack {
#ifndef NDEBUG
	lua_State *const L;
	const int expected_top;

public:
	explicit ScopeCheckStack(lua_State *_L, int offset = 0) noexcept
		:L(_L), expected_top(lua_gettop(L) + offset) {}

	~ScopeCheckStack() noexcept {
		assert(lua_gettop(L) == expected_top);
	}

#else
public:
	explicit constexpr ScopeCheckStack(lua_State *) noexcept {}
	constexpr ScopeCheckStack(lua_State *, int) noexcept {}
#endif

	ScopeCheckStack(const ScopeCheckStack &) = delete;
	ScopeCheckStack &operator=(const ScopeCheckStack &) = delete;
};

}
