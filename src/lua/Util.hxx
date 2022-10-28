/*
 * Copyright 2015-2022 Max Kellermann <max.kellermann@gmail.com>
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

#pragma once

#include "Assert.hxx"
#include "StackIndex.hxx"

extern "C" {
#include <lua.h>
}

#include <cstddef>
#include <string_view>

namespace Lua {

struct LightUserData {
	void *value;

	explicit constexpr LightUserData(void *_value) noexcept
		:value(_value) {}
};

static inline void
Push(lua_State *L, std::nullptr_t) noexcept
{
	lua_pushnil(L);
}

static inline void
Push(lua_State *L, StackIndex i) noexcept
{
	lua_pushvalue(L, i.idx);
}

[[gnu::nonnull]]
static inline void
Push(lua_State *L, bool value) noexcept
{
	lua_pushboolean(L, value);
}

[[gnu::nonnull]]
static inline void
Push(lua_State *L, const char *value) noexcept
{
	lua_pushstring(L, value);
}

[[gnu::nonnull]]
static inline void
Push(lua_State *L, std::string_view value) noexcept
{
	lua_pushlstring(L, value.data(), value.size());
}

[[gnu::nonnull]]
static inline void
Push(lua_State *L, lua_Integer value) noexcept
{
	lua_pushinteger(L, value);
}

[[gnu::nonnull]]
static inline void
Push(lua_State *L, double value) noexcept
{
	lua_pushnumber(L, value);
}

struct CurrentThread {};

static inline void
Push(lua_State *L, CurrentThread) noexcept
{
	lua_pushthread(L);
}

[[gnu::nonnull]]
static inline void
Push(lua_State *L, lua_CFunction value) noexcept
{
	lua_pushcfunction(L, value);
}

[[gnu::nonnull]]
static inline void
Push(lua_State *L, LightUserData value) noexcept
{
	lua_pushlightuserdata(L, value.value);
}

template<typename V>
void
SetGlobal(lua_State *L, const char *name, V &&value) noexcept
{
	Push(L, std::forward<V>(value));
	lua_setglobal(L, name);
}

template<typename I, typename K>
void
GetTable(lua_State *L, I idx, K &&key) noexcept
{
	const ScopeCheckStack check_stack(L, 1);

	Push(L, std::forward<K>(key));
	StackPushed(idx);
	lua_gettable(L, StackIndex{idx}.idx);
}

template<typename I, typename K, typename V>
void
SetTable(lua_State *L, I idx, K &&key, V &&value) noexcept
{
	const ScopeCheckStack check_stack(L);

	Push(L, std::forward<K>(key));
	StackPushed(value);
	Push(L, std::forward<V>(value));
	StackPushed(idx, 2);
	lua_settable(L, StackIndex{idx}.idx);
}

template<typename I, typename V>
void
SetField(lua_State *L, I idx, const char *name, V &&value) noexcept
{
	const ScopeCheckStack check_stack(L);

	Push(L, std::forward<V>(value));
	StackPushed(idx);
	lua_setfield(L, StackIndex{idx}.idx, name);
}

template<typename V>
static inline void
SetRegistry(lua_State *L, const char *name, V &&value) noexcept
{
	SetField(L, LUA_REGISTRYINDEX, name, std::forward<V>(value));
}

static inline void *
GetRegistryLightUserData(lua_State *L, const char *name) noexcept
{
	const ScopeCheckStack check_stack(L);

	lua_getfield(L, LUA_REGISTRYINDEX, name);
	void *value = lua_touserdata(L, -1);
	lua_pop(L, 1);
	return value;
}

template<typename V>
static inline void
SetField(lua_State *L, const char *package,
	 const char *name, V &&value) noexcept
{
	const ScopeCheckStack check_stack(L);

	lua_getglobal(L, package);
	StackPushed(value);
	SetField(L, RelativeStackIndex{-1}, name, std::forward<V>(value));
	lua_pop(L, 1);
}

/**
 * Sets the variable "package.path", which controls the package
 * search path for the "require" command.
 */
[[gnu::nonnull]]
static inline void
SetPackagePath(lua_State *L, const char *path) noexcept
{
	SetField(L, "package", "path", path);
}

}
