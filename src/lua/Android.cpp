// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The XCSoar Project

#include "Android.hpp"
#include "Util.hxx"
#include "Android/Main.hpp"
#include "Android/NativeView.hpp"

extern "C" {
#include <lauxlib.h>
}

static int
l_share_text(lua_State *L)
{
  if (lua_gettop(L) != 1)
    return luaL_error(L, "Invalid parameters");

  const char *text = lua_tostring(L, 1);
  if (text == nullptr)
    return luaL_argerror(L, 1, "String expected");

  native_view->ShareText(Java::GetEnv(), text);
  return 0;
}

void
Lua::InitAndroid(lua_State *L)
{
  lua_getglobal(L, "xcsoar");
  SetField(L, RelativeStackIndex{-1},
           "share_text", l_share_text);
  lua_pop(L, 1);
}
