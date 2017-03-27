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

#include "Settings.hpp"
#include "Util.hxx"
#include "Util/StringAPI.hxx"
#include "Interface.hpp"
#include "ActionInterface.hpp"

extern "C" {
#include <lauxlib.h>
}

static int
l_settings_index(lua_State *L)
{
  const char *name = lua_tostring(L, 2);
  if (name == nullptr)
    return 0;
  else if (StringIsEqual(name, "mc")) {
    const ComputerSettings &settings_computer =
      CommonInterface::GetComputerSettings();
    
    Lua::Push(L, settings_computer.polar.glide_polar_task.GetMC());
  } else if (StringIsEqual(name, "bugs")) {
      /* How clean the glider is. */
      const ComputerSettings &settings_computer = 
        CommonInterface::GetComputerSettings();
    
      Lua::Push(L, settings_computer.polar.bugs);
  } else if (StringIsEqual(name, "wingload")) {
      /* Current used wingload */
      const ComputerSettings &settings_computer =
        CommonInterface::GetComputerSettings();
    
      Lua::Push(L, settings_computer.polar.glide_polar_task.GetWingLoading());
  } else if (StringIsEqual(name, "ballast")) {
      /* Ballast of the glider */
      const ComputerSettings &settings_computer =
        CommonInterface::GetComputerSettings();
    
      Lua::Push(L, settings_computer.polar.glide_polar_task.GetBallast());
  } else if (StringIsEqual(name, "qnh")) {
      /* Area pressure for barometric altimeter calibration */
      const ComputerSettings &settings_computer =
        CommonInterface::GetComputerSettings();
    
      Lua::Push(L, settings_computer.pressure.GetPascal());
  } else if (StringIsEqual(name, "max_temp")) {
      /* The forecast ground temperature.  Used by 
         convection estimator. */
      const ComputerSettings &settings_computer =
        CommonInterface::GetComputerSettings();
    
      Lua::Push(L, settings_computer.forecast_temperature.ToKelvin());
  } else if (StringIsEqual(name, "safetymc")) {
      /* The MacCready setting used, when safety MC is enabled 
         for reach calculations, in task abort mode and for 
         determining arrival altitude at airfields. */
      const ComputerSettings &settings_computer =
        CommonInterface::GetComputerSettings();
      const TaskBehaviour &task_behaviour = settings_computer.task;
    
      Lua::Push(L, task_behaviour.safety_mc);
  } else if (StringIsEqual(name, "riskfactor")) {
      /* The STF risk factor reduces the MacCready setting used to 
         calculate speed to fly as the glider gets low, in order to 
         compensate for risk. Set to 0.0 for no compensation, 
         1.0 scales MC linearly with current height (with reference 
         to height of the maximum climb). If considered, 0.3 is recommended. */
      const ComputerSettings &settings_computer =
        CommonInterface::GetComputerSettings();
      const TaskBehaviour &task_behaviour = settings_computer.task;
    
      Lua::Push(L, task_behaviour.risk_gamma);
  } else if (StringIsEqual(name, "polardegradation")) {
      /* A permanent polar degradation, 0% means no degradation, 
         50% indicates the glider's sink rate is doubled. */
      const ComputerSettings &settings_computer =
        CommonInterface::GetComputerSettings();
    
      Lua::Push(L, settings_computer.polar.degradation_factor);
  } else if (StringIsEqual(name, "arrivalheight")) {
      /* The height above terrain that the glider should arrive 
         at for a safe landing. */
      const ComputerSettings &settings_computer =
        CommonInterface::GetComputerSettings();
      const TaskBehaviour &task_behaviour = settings_computer.task;
    
      Lua::Push(L, task_behaviour.safety_height_arrival);
  } else if (StringIsEqual(name, "terrainheight")) {
      /* The height above terrain that the glider must clear during 
         final glide. */
      const ComputerSettings &settings_computer =
        CommonInterface::GetComputerSettings();
      const TaskBehaviour &task_behaviour = settings_computer.task;
    
      Lua::Push(L, task_behaviour.route_planner.safety_height_terrain);
  } else
    return 0;

  return 1;
}

static int
l_settings_setmc(lua_State *L)
{
  if (lua_gettop(L) != 1)
    return luaL_error(L, "Invalid parameters");

  ActionInterface::SetMacCready(luaL_checknumber(L, 1), false);
  return 0;
}

static int
l_settings_setbugs(lua_State *L)
{
  if (lua_gettop(L) != 1)
    return luaL_error(L, "Invalid parameters");

  ActionInterface::SetBugs(luaL_checknumber(L, 1));
  return 0;
}

static int
l_settings_setqnh(lua_State *L)
{
  if (lua_gettop(L) != 1)
    return luaL_error(L, "Invalid parameters");

  const NMEAInfo &basic = CommonInterface::Basic();
  ComputerSettings &settings_computer = CommonInterface::SetComputerSettings();

  settings_computer.pressure = AtmosphericPressure::Pascal(luaL_checknumber(L, 1));
  settings_computer.pressure_available.Update(basic.clock);
  return 0;
}

static int
l_settings_setballast(lua_State *L)
{
  if (lua_gettop(L) != 1)
    return luaL_error(L, "Invalid parameters");

  ActionInterface::SetBallast(luaL_checknumber(L, 1));
  return 0;
}

static int
l_settings_setmaxtemp(lua_State *L)
{
  if (lua_gettop(L) != 1)
    return luaL_error(L, "Invalid parameters");

  ComputerSettings &settings = CommonInterface::SetComputerSettings();
  settings.forecast_temperature = Temperature::FromKelvin(luaL_checknumber(L, 1));
  return 0;
}

static constexpr struct luaL_Reg settings_funcs[] = {
  {"setmc", l_settings_setmc},
  {"setbugs", l_settings_setbugs},
  {"setqnh", l_settings_setqnh},
  {"setballast", l_settings_setballast},
  {"setmaxtemp", l_settings_setmaxtemp},
  {nullptr, nullptr}
};

void
Lua::InitSettings(lua_State *L)
{
  lua_getglobal(L, "xcsoar");

  lua_newtable(L);

  lua_newtable(L);
  SetField(L, -2, "__index", l_settings_index);
  lua_setmetatable(L, -2);

  luaL_setfuncs(L, settings_funcs, 0);

  lua_setfield(L, -2, "settings");

  lua_pop(L, 1);
}
