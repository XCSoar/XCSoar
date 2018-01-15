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

#ifndef LUA_VALUE_HXX
#define LUA_VALUE_HXX

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
	void Set(V &&value) {
		SetTable(L, LUA_REGISTRYINDEX,
			 LightUserData(this),
			 std::forward<V>(value));
	}

	/**
	 * Push the value on the stack.
	 */
	void Push() const {
		GetTable(L, LUA_REGISTRYINDEX,
			 LightUserData(const_cast<Value *>(this)));
	}

	lua_State *GetState() const {
		return L;
	}
};

static inline void
Push(lua_State *, const Value &value)
{
	value.Push();
}

inline
Value::Value(const Value &src)
	:Value(src.L, src) {}

}

#endif
