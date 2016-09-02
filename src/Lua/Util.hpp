/*
Copyright_License {

  XCSoar Glide Computer - http://www.xcsoar.org/
  Copyright (C) 2000-2016 The XCSoar Project
  A detailed list of copyright holders can be found in the file "AUTHORS".

  This program is free software; you can redistribute it and/or
  modify it under the terms of the GNU General Public License
  as published by the Free Software Foundation; either version 2
  of the License, or (at your option) any later version.

  This program is distributed in the hope that it will be useful,
  but WITHOUT ANY WARRANTY; without even the implied warranty of
  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
  GNU General Public License for more details.

  You should have received a copy of the GNU General Public License
  along with this program; if not, write to the Free Software
  Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA  02111-1307, USA.
}
*/

#ifndef XCSOAR_LUA_UTIL_HPP
#define XCSOAR_LUA_UTIL_HPP

#include "Compiler.h"

extern "C" {
#include <lua.h>
}

#include <cstddef>

namespace Lua {

struct LightUserData {
  void *value;

  constexpr LightUserData(void *_value):value(_value) {}
};

static inline void
Push(lua_State *L, std::nullptr_t)
{
  lua_pushnil(L);
}

gcc_nonnull_all
static inline void
Push(lua_State *L, bool value)
{
  lua_pushboolean(L, value);
}

gcc_nonnull_all
static inline void
Push(lua_State *L, const char *value)
{
  lua_pushstring(L, value);
}

gcc_nonnull_all
static inline void
Push(lua_State *L, int value)
{
  lua_pushinteger(L, value);
}

gcc_nonnull_all
static inline void
Push(lua_State *L, double value)
{
  lua_pushnumber(L, value);
}

gcc_nonnull_all
static inline void
Push(lua_State *L, lua_CFunction value)
{
  lua_pushcfunction(L, value);
}

gcc_nonnull_all
static inline void
Push(lua_State *L, LightUserData value)
{
  lua_pushlightuserdata(L, value.value);
}

template<typename V>
void
SetField(lua_State *L, int idx, const char *name, V &&value)
{
  Push(L, value);
  lua_setfield(L, idx, name);
}

template<typename V>
static inline void
SetRegistry(lua_State *L, const char *name, V &&value)
{
  SetField(L, LUA_REGISTRYINDEX, name, value);
}

static inline void *
GetRegistryLightUserData(lua_State *L, const char *name)
{
  lua_getfield(L, LUA_REGISTRYINDEX, name);
  void *value = lua_touserdata(L, -1);
  lua_pop(L, 1);
  return value;
}

template<typename V>
static inline void
SetField(lua_State *L, const char *package, const char *name, V &&value)
{
  lua_getglobal(L, package);
  SetField(L, -2, name, value);
  lua_pop(L, 1);
}

/**
 * Sets the variable "package.path", which controls the package
 * search path for the "require" command.
 */
gcc_nonnull_all
static inline void
SetPackagePath(lua_State *L, const char *path) {
  SetField(L, "package", "path", path);
}

template<typename F>
void ForEach(lua_State *L) {
}

}

#endif
