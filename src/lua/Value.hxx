// SPDX-License-Identifier: BSD-2-Clause
// author: Max Kellermann <max.kellermann@gmail.com>

#pragma once

#include "Util.hxx"

extern "C" {
#include <lua.h>
}

namespace Lua {

/**
 * Save a Lua value for using it later.
 */
class Value {
	lua_State *const L;

public:
	/**
	 * Initialize an instance with "nil".
	 */
	explicit Value(lua_State *_L):L(_L) {}

	/**
	 * Initialize an instance with an object on the stack.
	 */
	template<typename V>
	Value(lua_State *_L, V &&value):L(_L) {
		Set(std::forward<V>(value));
	}

	Value(const Value &src);

	~Value() {
		Set(nullptr);
	}

	Value &operator=(const Value &) = delete;

	template<typename V>
	void Set(lua_State *thread_L, V &&value) {
		SetTable(thread_L, LUA_REGISTRYINDEX,
			 LightUserData(this),
			 std::forward<V>(value));
	}

	template<typename V>
	void Set(V &&value) {
		Set(L, std::forward<V>(value));
	}

	/**
	 * Push the value on the given thread's stack.
	 */
	void Push(lua_State *thread_L) const {
		GetTable(thread_L, LUA_REGISTRYINDEX,
			 LightUserData(const_cast<Value *>(this)));
	}

	/**
	 * Push the value on the stack.
	 */
	void Push() const {
		Push(L);
	}

	lua_State *GetState() const noexcept {
		return L;
	}
};

static inline void
Push(lua_State *L, const Value &value)
{
	value.Push(L);
}

inline
Value::Value(const Value &src)
	:Value(src.L, src) {}

}
