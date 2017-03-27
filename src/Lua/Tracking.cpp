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

#include "Tracking.hpp"
#include "Util.hxx"
#include "Util/StringAPI.hxx"
#include "Interface.hpp"

extern "C" {
#include <lauxlib.h>
}

static int
l_tracking_index(lua_State *L)
{
  const TrackingSettings &settings =
    CommonInterface::GetComputerSettings().tracking;
  const char *name = lua_tostring(L, 2);
  if (name == nullptr)
    return 0;
  else if (StringIsEqual(name, "skylines_enabled")) {
    Lua::Push(L, settings.skylines.enabled);
  } else if (StringIsEqual(name, "skylines_roaming")) {
      Lua::Push(L, settings.skylines.roaming);
  } else if (StringIsEqual(name, "skylines_interval")) {
      Lua::Push(L, (int)settings.skylines.interval);
  } else if (StringIsEqual(name, "skylines_traffic_enabled")) {
      /* Track friends, downloads the position of your 
         friends live from the SkyLines server. */
      Lua::Push(L, settings.skylines.traffic_enabled);
  } else if (StringIsEqual(name, "skylines_near_traffic_enabled")) {
      /* Show nearby traffic, downloads the position 
         of your nearby traffic live from the SkyLines server. */
      Lua::Push(L, settings.skylines.near_traffic_enabled);
  } else if (StringIsEqual(name, "livetrack24_enabled")) {
      Lua::Push(L, settings.livetrack24.enabled);
  } else if (StringIsEqual(name, "livetrack24_interval")) {
      Lua::Push(L, (int)settings.interval);
  } else if (StringIsEqual(name, "livetrack24_vehicle_name")) { 
      Lua::Push(L, settings.vehicle_name);
  } else
    return 0;

  return 1;
}


static int
l_tracking_enable_skylines(lua_State *L)
{ 
  if (lua_gettop(L) != 0)
    return luaL_error(L, "Invalid parameters");

  TrackingSettings &settings =
    CommonInterface::SetComputerSettings().tracking;
  settings.skylines.enabled = true;  
  
  return 0;
}

static int
l_tracking_disable_skylines(lua_State *L)
{ 
  if (lua_gettop(L) != 0)
    return luaL_error(L, "Invalid parameters");

  TrackingSettings &settings =
    CommonInterface::SetComputerSettings().tracking;
  settings.skylines.enabled = false;  
  
  return 0;
}

static int
l_tracking_set_skylines_interval(lua_State *L)
{ 
  if (lua_gettop(L) != 1)
    return luaL_error(L, "Invalid parameters");

  TrackingSettings &settings =
    CommonInterface::SetComputerSettings().tracking;
  settings.skylines.interval = luaL_checknumber(L, 1); 
  
  return 0;
}

static int
l_tracking_enable_skylines_traffic(lua_State *L)
{ 
  if (lua_gettop(L) != 0)
    return luaL_error(L, "Invalid parameters");

  TrackingSettings &settings =
    CommonInterface::SetComputerSettings().tracking;
  settings.skylines.traffic_enabled = true;  
  
  return 0;
}

static int
l_tracking_disable_skylines_traffic(lua_State *L)
{ 
  if (lua_gettop(L) != 0)
    return luaL_error(L, "Invalid parameters");

  TrackingSettings &settings =
    CommonInterface::SetComputerSettings().tracking;
  settings.skylines.traffic_enabled = false;  
  
  return 0;
}

static int
l_tracking_enable_skylines_neartraffic(lua_State *L)
{ 
  if (lua_gettop(L) != 0)
    return luaL_error(L, "Invalid parameters");

  TrackingSettings &settings =
    CommonInterface::SetComputerSettings().tracking;
  settings.skylines.near_traffic_enabled = true;  
  
  return 0;
}

static int
l_tracking_disable_skylines_neartraffic(lua_State *L)
{ 
  if (lua_gettop(L) != 0)
    return luaL_error(L, "Invalid parameters");

  TrackingSettings &settings =
    CommonInterface::SetComputerSettings().tracking;
  settings.skylines.near_traffic_enabled = false;  
  
  return 0;
}

static int
l_tracking_enable_livetrack24(lua_State *L)
{ 
  if (lua_gettop(L) != 0)
    return luaL_error(L, "Invalid parameters");

  TrackingSettings &settings =
    CommonInterface::SetComputerSettings().tracking;
  settings.livetrack24.enabled = true;  
  
  return 0;
}

static int
l_tracking_disable_livetrack24(lua_State *L)
{ 
  if (lua_gettop(L) != 0)
    return luaL_error(L, "Invalid parameters");

  TrackingSettings &settings =
    CommonInterface::SetComputerSettings().tracking;
  settings.livetrack24.enabled = false;  
  
  return 0;
}

static int
l_tracking_set_livetrack24_interval(lua_State *L)
{ 
  if (lua_gettop(L) != 1)
    return luaL_error(L, "Invalid parameters");

  TrackingSettings &settings =
    CommonInterface::SetComputerSettings().tracking;
  settings.interval = luaL_checknumber(L, 1); 
  
  return 0;
}

static int
l_tracking_set_livetrack24_vehiclename(lua_State *L)
{ 
  if (lua_gettop(L) != 1)
    return luaL_error(L, "Invalid parameters");

  TrackingSettings &settings =
    CommonInterface::SetComputerSettings().tracking;
  settings.vehicle_name.SetUTF8(luaL_checkstring(L, 1)); 
  
  return 0;
}

static constexpr struct luaL_Reg settings_funcs[] = {
  {"enable_skylines", l_tracking_enable_skylines},
  {"disable_skylines", l_tracking_disable_skylines},
  {"set_skylines_interval", l_tracking_set_skylines_interval},
  {"enable_skylines_traffic", l_tracking_enable_skylines_traffic},
  {"disable_skylines_traffic", l_tracking_disable_skylines_traffic},
  {"enable_skylines_near_traffic", l_tracking_enable_skylines_neartraffic},
  {"disable_skylines_near_traffic", l_tracking_disable_skylines_neartraffic},
  {"enable_livetrack24", l_tracking_enable_livetrack24},
  {"disable_livetrack24", l_tracking_disable_livetrack24},
  {"set_livetrack24_interval", l_tracking_set_livetrack24_interval},
  {"set_livetrack24_vehiclename", l_tracking_set_livetrack24_vehiclename},
  {nullptr, nullptr}
};

void
Lua::InitTracking(lua_State *L)
{
  lua_getglobal(L, "xcsoar");

  lua_newtable(L);

  lua_newtable(L);
  SetField(L, -2, "__index", l_tracking_index);
  lua_setmetatable(L, -2);

  luaL_setfuncs(L, settings_funcs, 0);

  lua_setfield(L, -2, "tracking");

  lua_pop(L, 1);
}
