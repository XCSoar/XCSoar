/*
Copyright_License {

  XCSoar Glide Computer - http://www.xcsoar.org/
  Copyright (C) 2000-2021 The XCSoar Project
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
