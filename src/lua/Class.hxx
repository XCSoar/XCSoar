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

#ifndef LUA_OBJECT_HXX
#define LUA_OBJECT_HXX

#include "Util.hxx"
#include "Assert.hxx"

extern "C" {
#include <lauxlib.h>
}

#include <new>
#include <type_traits>

namespace Lua {

/**
 * Helper to wrap a C++ class in a Lua metatable.  This allows
 * instantiating C++ objects managed by Lua.
 */
template<typename T, const char *name>
struct Class {
	using value_type = T;
	using pointer = T *;
	using reference = T &;

	/**
	 * Register the Lua metatable and leave it on the stack.  This
	 * must be called once before New().
	 */
	static void Register(lua_State *L) {
		const ScopeCheckStack check_stack(L, 1);

		luaL_newmetatable(L, name);

		/* let Lua's garbage collector call the destructor
		   (but only if there is one) */
		if (!std::is_trivially_destructible<T>::value)
			SetField(L, -2, "__gc", l_gc);
	}

	/**
	 * Create a new instance, push it on the Lua stack and return
	 * the native pointer.  It will be deleted automatically by
	 * Lua's garbage collector.
	 *
	 * You must call Register() once before calling this method.
	 */
	template<typename... Args>
	static pointer New(lua_State *L, Args&&... args) {
		const ScopeCheckStack check_stack(L, 1);

		void *p = lua_newuserdata(L, sizeof(value_type));
		luaL_getmetatable(L, name);
		lua_setmetatable(L, -2);

		try {
			return ::new(p) T(std::forward<Args>(args)...);
		} catch (...) {
			lua_pop(L, 1);
			throw;
		}
	}

	/**
	 * Extract the native pointer from the Lua object on the
	 * stack.  Returns nullptr if the type is wrong.
	 */
	[[gnu::pure]]
	static pointer Check(lua_State *L, int idx) {
		const ScopeCheckStack check_stack(L);

		void *p = lua_touserdata(L, idx);
		if (p == nullptr)
			return nullptr;

		if (lua_getmetatable(L, idx) == 0)
			return nullptr;

		lua_getfield(L, LUA_REGISTRYINDEX, name);
		bool equal = lua_rawequal(L, -1, -2);
		lua_pop(L, 2);
		if (!equal)
			return nullptr;

		return pointer(p);
	}

	/**
	 * Extract the native value from the Lua object on the
	 * stack.  Raise a Lua error if the type is wrong.
	 */
	[[gnu::pure]]
	static reference Cast(lua_State *L, int idx) {
		return *(pointer)luaL_checkudata(L, idx, name);
	}

private:
	static int l_gc(lua_State *L) {
		const ScopeCheckStack check_stack(L);

		auto *p = Check(L, 1);
		/* call the destructor when this instance is
		   garbage-collected */
		p->~T();
		return 0;
	}
};

}

#endif
