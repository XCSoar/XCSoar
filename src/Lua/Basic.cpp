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

#include "Basic.hpp"
#include "Util.hxx"
#include "Version.hpp"
#include "Util/ConvertString.hpp"

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

  SetField(L, -2, "VERSION", WideToUTF8Converter(XCSoar_Version));

  lua_setglobal(L, "xcsoar");

  return L;
}
