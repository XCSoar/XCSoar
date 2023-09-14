// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The XCSoar Project

#include "Basic.hpp"
#include "Util.hxx"
#include "Version.hpp"

extern "C" {
#include <lauxlib.h>
#include <lualib.h>
}

/* table copied from Lua's linit.c, with some modules disabled */
static constexpr luaL_Reg loadedlibs[] = {
  {"_G", luaopen_base},
  {LUA_LOADLIBNAME, luaopen_package},
  //{LUA_COLIBNAME, luaopen_coroutine},
  {LUA_TABLIBNAME, luaopen_table},
  //{LUA_IOLIBNAME, luaopen_io},
  //{LUA_OSLIBNAME, luaopen_os},
  {LUA_STRLIBNAME, luaopen_string},
  {LUA_MATHLIBNAME, luaopen_math},
  //{LUA_UTF8LIBNAME, luaopen_utf8},
  //{LUA_DBLIBNAME, luaopen_debug},
#if defined(LUA_COMPAT_BITLIB)
  //{LUA_BITLIBNAME, luaopen_bit32},
#endif
};

lua_State *
Lua::NewBasicState()
{
  lua_State *L = luaL_newstate();

  SetRegistry(L, "LUA_NOENV", true);

  for (auto l : loadedlibs) {
    luaL_requiref(L, l.name, l.func, 1);
    lua_pop(L, 1);
  }

  /* create the "xcsoar" namespace */
  lua_newtable(L);

  SetField(L, RelativeStackIndex{-1}, "VERSION", XCSoar_Version);

  lua_setglobal(L, "xcsoar");

  return L;
}
