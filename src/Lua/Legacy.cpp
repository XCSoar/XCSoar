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

#include "Legacy.hpp"
#include "Util.hxx"
#include "Input/InputLookup.hpp"
#include "Util/ConvertString.hpp"

extern "C" {
#include <lauxlib.h>
}

static int
l_fire_legacy_event(lua_State *L)
{
  const char *event = lua_tostring(L, 1);

  if (event == nullptr)
    return luaL_error(L, "No InputEvent specified");

  auto *event_function = InputEvents::findEvent(UTF8ToWideConverter(event));
  if (event_function == nullptr)
    return luaL_error(L, "Unknown InputEvent");

  const char *parameter = lua_tostring(L, 2);
  if (parameter == nullptr)
    parameter = "";

  event_function(UTF8ToWideConverter(parameter));
  return 0;
}

void
Lua::InitLegacy(lua_State *L)
{
  lua_getglobal(L, "xcsoar");
  SetField(L, -2, "fire_legacy_event", l_fire_legacy_event);
  lua_pop(L, 1);
}
