// SPDX-License-Identifier: BSD-2-Clause
// author: Max Kellermann <max.kellermann@gmail.com>

#pragma once

extern "C" {
#include <lua.h>
}

#ifndef NDEBUG
#include <cassert>
#ifdef LUA_LJDIR
#include <exception>
#endif
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
#ifdef LUA_LJDIR
		if (std::uncaught_exceptions() == 0) {
#endif
			assert(lua_gettop(L) == expected_top);
#ifdef LUA_LJDIR
		} else {
			/* if we are unwinding the stack due to
			   lua_error() (LuaJit only), then the error
			   was put on the Lua stack, but if this is a
			   C++ exception, there is no error on the Lua
			   stack; since std::current_exception() does
			   not work here, we can't know the
			   difference, so this assert() allows both */
			assert(lua_gettop(L) == expected_top ||
			       lua_gettop(L) == expected_top + 1);
		}
#endif
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
