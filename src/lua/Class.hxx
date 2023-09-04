// SPDX-License-Identifier: BSD-2-Clause
// author: Max Kellermann <max.kellermann@gmail.com>

#pragma once

#include "Util.hxx"
#include "Assert.hxx"

extern "C" {
#include <lauxlib.h>
}

#ifdef LUA_LJDIR
#include <exception>
#endif

#include <memory>
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
		if constexpr (!std::is_trivially_destructible_v<T>)
			SetField(L, RelativeStackIndex{-1}, "__gc", l_gc);
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
		ScopeCheckStack check_stack{L};

		T *p = static_cast<T *>(lua_newuserdata(L, sizeof(value_type)));

		try {
			p = std::construct_at(p, std::forward<Args>(args)...);

			/* apply the metatable only after the
			   constructor finishes successfully; the Lua
			   GC must not call the destructor if the
			   constructor has thrown an exception */
			luaL_setmetatable(L, name);

			++check_stack;
			return p;
		} catch (...) {
			/* pop the new object, its construction has
			   failed, we must not use it */

#ifdef LUA_LJDIR
			if (std::current_exception()) {
#endif
				/* this is a C++ exception */
				lua_pop(L, 1);
#ifdef LUA_LJDIR
			} else {
				/* this is a lua_error() (only
				   supported on LuaJit) and the error
				   is on the top of the Lua stack; we
				   need to use lua_remove() to remove
				   the new object behind it */
				lua_remove(L, -2);
			}
#endif

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

		T *p = static_cast<T *>(lua_touserdata(L, 1));
		/* call the destructor when this instance is
		   garbage-collected */
		std::destroy_at(p);
		return 0;
	}
};

}
