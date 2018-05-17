/*
 * Copyright (C) 2017 Max Kellermann <max.kellermann@gmail.com>
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 *
 * - Redistributions of source code must retain the above copyright
 * notice, this list of conditions and the following disclaimer.
 *
 * - Redistributions in binary form must reproduce the above copyright
 * notice, this list of conditions and the following disclaimer in the
 * documentation and/or other materials provided with the
 * distribution.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
 * ``AS IS'' AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
 * LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS
 * FOR A PARTICULAR PURPOSE ARE DISCLAIMED.  IN NO EVENT SHALL THE
 * FOUNDATION OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT,
 * INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES
 * (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR
 * SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT,
 * STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
 * ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED
 * OF THE POSSIBILITY OF SUCH DAMAGE.
 */

#ifndef LUA_ASSERT_HXX
#define LUA_ASSERT_HXX

extern "C" {
#include <lua.h>
}

#ifndef NDEBUG
#include <assert.h>
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
	explicit ScopeCheckStack(lua_State *_L, int offset = 0)
		:L(_L), expected_top(lua_gettop(L) + offset) {}

	~ScopeCheckStack() {
		assert(lua_gettop(L) == expected_top);
	}

#else
public:
	explicit ScopeCheckStack(lua_State *) {}
	ScopeCheckStack(lua_State *, int) {}
#endif

	ScopeCheckStack(const ScopeCheckStack &) = delete;
	ScopeCheckStack &operator=(const ScopeCheckStack &) = delete;
};

}

#endif
