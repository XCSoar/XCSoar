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

#include "Wind.hpp"
#include "Geo.hpp"
#include "Math/Angle.hpp"
#include "Util.hxx"
#include "Util/StringAPI.hxx"
#include "Interface.hpp"

extern "C" {
#include <lauxlib.h>
}

static int
l_wind_index(lua_State *L)
{
  const char *name = lua_tostring(L, 2);
  if (name == nullptr)
    return 0;
  else if (StringIsEqual(name, "wind_mode")) {
    /* Wind mode
       0: Manual, When the algorithm is switched off, the pilot is 
          responsible for setting the wind estimate.
       1: Circling, Requires only a GPS source.
       2: ZigZag, Requires GPS and an intelligent vario with airspeed output.
       3: Both, Uses ZigZag and circling. */
    const WindSettings &settings = CommonInterface::GetComputerSettings().wind;
    Lua::Push(L, (int)settings.GetLegacyAutoWindMode());
  } else if (StringIsEqual(name, "manual_wind_bearing")) {
      SpeedVector manual_wind = CommonInterface::Calculated().GetWindOrZero();
      Lua::Push(L, manual_wind.bearing);
  } else if (StringIsEqual(name, "manual_wind_speed")) {
      SpeedVector manual_wind = CommonInterface::Calculated().GetWindOrZero();
      Lua::Push(L, manual_wind.norm);
  } else if (StringIsEqual(name, "tail_drift")) {
      /* Determines whether the snail trail is drifted with the wind 
         when displayed in circling mode. Switched Off, "
         the snail trail stays uncompensated for wind drift. */
      MapSettings &map_settings = CommonInterface::SetMapSettings();
      if (map_settings.trail.wind_drift_enabled) Lua::Push(L, 1);
      else Lua::Push(L, 0);
  } else if (StringIsEqual(name, "wind_source")) {
      /* The Source of the current wind
         0: None
         1: Manual
         2: Circling
         3: ZigZag
         4: External */
      const DerivedInfo &calculated = CommonInterface::Calculated();
      Lua::Push(L, (int)calculated.wind_source);
  } else if (StringIsEqual(name, "wind_speed")) {
      // The current wind speed [m/s] 
      const DerivedInfo &info = CommonInterface::Calculated();
      Lua::Push(L, info.wind.norm);
  } else if (StringIsEqual(name, "wind_bearing")) {
      // The current wind bearing [degrees]
      const DerivedInfo &info = CommonInterface::Calculated();
      Lua::Push(L, info.wind.bearing);
  } else
    return 0;

  return 1;
}

static int
l_wind_setwindmode(lua_State *L)
{
  /* Sets the wind mode 
     0: Manual
     1: Circling
     2: ZigZag 
     3: Both */
  if (lua_gettop(L) != 1)
    return luaL_error(L, "Invalid parameters");

  WindSettings &settings = CommonInterface::SetComputerSettings().wind;

  int mode = luaL_checknumber(L, 1);
  if ((mode < 4) && (mode >= 0))
    settings.SetLegacyAutoWindMode(mode); 
  return 0;
}

static int
l_wind_settaildrift(lua_State *L)
{
  /* Turns the taildrift Off / On
     0: Off / 1: On */
  if (lua_gettop(L) != 1)
    return luaL_error(L, "Invalid parameters");

  MapSettings &map_settings = CommonInterface::SetMapSettings();
  int tail = luaL_checknumber(L, 1);     
  if (tail) map_settings.trail.wind_drift_enabled = true;
  else map_settings.trail.wind_drift_enabled = false;

  return 0;
}

static int
l_wind_setwindspeed(lua_State *L)
{
  // Sets manual the wind speed [m/s]
  if (lua_gettop(L) != 1)
    return luaL_error(L, "Invalid parameters");

  const NMEAInfo &basic = CommonInterface::Basic();
  WindSettings &settings = CommonInterface::SetComputerSettings().wind;
  float speed = luaL_checknumber(L, 1); 
  
  if (speed >= 0) {
    settings.manual_wind.norm = speed;
    settings.manual_wind_available.Update(basic.clock);
  }

  return 0;
}

static int
l_wind_setwindbearing(lua_State *L)
{
  // Sets manual the wind bearing [degress]
  if (lua_gettop(L) != 1)
    return luaL_error(L, "Invalid parameters");

  const NMEAInfo &basic = CommonInterface::Basic();
  WindSettings &settings = CommonInterface::SetComputerSettings().wind;
  float direction = luaL_checknumber(L, 1); 
  
  settings.manual_wind.bearing = Angle::Degrees(direction);
  settings.manual_wind_available.Update(basic.clock);
  
  return 0;
}

static int
l_wind_clear(lua_State *L)
{
  // Clears all wind calculations
  if (lua_gettop(L) != 0)
    return luaL_error(L, "Invalid parameters");

  CommonInterface::SetComputerSettings().wind.manual_wind_available.Clear();
  
  return 0;
}

static constexpr struct luaL_Reg settings_funcs[] = {
  {"setwindmode", l_wind_setwindmode},
  {"settaildrift", l_wind_settaildrift},
  {"setwindspeed", l_wind_setwindspeed},
  {"setwindbearing", l_wind_setwindbearing},
  {"clear", l_wind_clear},
  {nullptr, nullptr}
};

void
Lua::InitWind(lua_State *L)
{
  lua_getglobal(L, "xcsoar");

  lua_newtable(L);

  lua_newtable(L);
  SetField(L, -2, "__index", l_wind_index);
  lua_setmetatable(L, -2);

  luaL_setfuncs(L, settings_funcs, 0);

  lua_setfield(L, -2, "wind");

  lua_pop(L, 1);
}
