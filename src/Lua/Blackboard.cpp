/*
Copyright_License {

  XCSoar Glide Computer - http://www.xcsoar.org/
  Copyright (C) 2000-2015 The XCSoar Project
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

#include "Blackboard.hpp"
#include "Geo.hpp"
#include "Util.hpp"
#include "Util/StringAPI.hxx"
#include "Interface.hpp"

namespace Lua {
  template<typename V>
  static void PushOptional(lua_State *L, bool available, V &&value) {
    if (available)
      Push(L, value);
    else
      lua_pushnil(L);
  }
}

static int
l_blackboard_index(lua_State *L)
{
  const auto &basic = CommonInterface::Basic();

  const char *name = lua_tostring(L, 2);
  if (name == nullptr)
    return 0;
  else if (StringIsEqual(name, "location"))
    Lua::PushOptional(L, basic.location_available, basic.location);
  else if (StringIsEqual(name, "altitude"))
    Lua::PushOptional(L, basic.NavAltitudeAvailable(), basic.nav_altitude);
  else if (StringIsEqual(name, "track"))
    Lua::PushOptional(L, basic.track_available, basic.track);
  else if (StringIsEqual(name, "ground_speed"))
    Lua::PushOptional(L, basic.ground_speed_available, basic.ground_speed);
  else if (StringIsEqual(name, "air_speed"))
    Lua::PushOptional(L, basic.airspeed_available, basic.true_airspeed);
  else
    return 0;

  return 1;
}

void
Lua::InitBlackboard(lua_State *L)
{
  lua_getglobal(L, "xcsoar");

  lua_newtable(L);

  lua_newtable(L);
  SetField(L, -2, "__index", l_blackboard_index);
  lua_setmetatable(L, -2);

  lua_setfield(L, -2, "blackboard");

  lua_pop(L, 1);
}
