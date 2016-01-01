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

#include "Logger.hpp"
#include "Util.hpp"
#include "Interface.hpp"

extern "C" {
#include <lauxlib.h>
}

static int
l_logger_index(lua_State *L)
{
  const ComputerSettings &settings_computer = CommonInterface::GetComputerSettings();
  const LoggerSettings &logger = settings_computer.logger;

  const char *name = lua_tostring(L, 2);
  if (name == nullptr)
    return 0;
  else if (StringIsEqual(name, "pilot_name")) { 
    Lua::Push(L, logger.pilot_name);
  } else
    return 0;

  return 1;
}


static int
l_logger_setpilotname(lua_State *L)
{
  if (lua_gettop(L) != 1)
    return luaL_error(L, "Invalid parameters");

  ComputerSettings &settings_computer = CommonInterface::SetComputerSettings();
  LoggerSettings &logger = settings_computer.logger;
  logger.pilot_name.SetUTF8(luaL_checkstring(L, 1));
  
  return 0;
}

static constexpr struct luaL_Reg settings_funcs[] = {
  {"set_pilot_name", l_logger_setpilotname},
  {nullptr, nullptr}
};

void
Lua::InitLogger(lua_State *L)
{
  lua_getglobal(L, "xcsoar");

  lua_newtable(L);

  lua_newtable(L);
  SetField(L, -2, "__index", l_logger_index);
  lua_setmetatable(L, -2);

  luaL_setfuncs(L, settings_funcs, 0);

  lua_setfield(L, -2, "logger");

  lua_pop(L, 1);
}
