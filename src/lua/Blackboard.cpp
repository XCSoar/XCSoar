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

#include "Blackboard.hpp"
#include "Chrono.hpp"
#include "Geo.hpp"
#include "MetaTable.hxx"
#include "Util.hxx"
#include "util/StringAPI.hxx"
#include "Interface.hpp"

namespace Lua {

static void
Push(lua_State *L, const BrokenDateTime &dt)
{
  lua_newtable(L);

  if (dt.IsDatePlausible()) {
    SetField(L, RelativeStackIndex{-1}, "year", dt.year);
    SetField(L, RelativeStackIndex{-1}, "month", dt.month);
    SetField(L, RelativeStackIndex{-1}, "day", dt.day);

    if (dt.day_of_week >= 0)
      SetField(L, RelativeStackIndex{-1}, "wday", dt.day_of_week + 1);
  }

  SetField(L, RelativeStackIndex{-1}, "hour", dt.hour);
  SetField(L, RelativeStackIndex{-1}, "min", dt.minute);
  SetField(L, RelativeStackIndex{-1}, "sec", dt.second);
}

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
  else if (StringIsEqual(name, "clock"))
    Lua::Push(L, basic.clock);
  else if (StringIsEqual(name, "time"))
    Lua::PushOptional(L, basic.time_available, basic.time);
  else if (StringIsEqual(name, "date_time_utc"))
    Lua::PushOptional(L, basic.time_available, basic.date_time_utc);
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
  else if (StringIsEqual(name, "bank_angle"))
    Lua::PushOptional(L, basic.attitude.IsBankAngleUseable(), basic.attitude.bank_angle);
  else if (StringIsEqual(name, "pitch_angle"))
    Lua::PushOptional(L, basic.attitude.IsPitchAngleUseable(), basic.attitude.pitch_angle); 
  else if (StringIsEqual(name, "heading"))
    Lua::PushOptional(L, basic.attitude.IsHeadingUseable(), basic.attitude.heading);
  else if (StringIsEqual(name, "g_load"))
    Lua::PushOptional(L, basic.acceleration.available, basic.acceleration.g_load);
  else if (StringIsEqual(name, "static_pressure"))
    Lua::PushOptional(L, basic.static_pressure_available, basic.static_pressure.GetPascal());
  else if (StringIsEqual(name, "pitot_pressure"))
    Lua::PushOptional(L, basic.pitot_pressure_available, basic.pitot_pressure.GetPascal());
  else if (StringIsEqual(name, "dynamic_pressure"))
    Lua::PushOptional(L, basic.dyn_pressure_available, basic.dyn_pressure.GetPascal());
  else if (StringIsEqual(name, "temperature"))
    Lua::PushOptional(L, basic.temperature_available,
                      basic.temperature.ToKelvin());
  else if (StringIsEqual(name, "humidity"))
    Lua::PushOptional(L, basic.humidity_available, basic.humidity);
  else if (StringIsEqual(name, "voltage"))
    Lua::PushOptional(L, basic.voltage_available, basic.voltage);
  else if (StringIsEqual(name, "battery_level"))
    Lua::PushOptional(L, basic.battery_level_available, basic.battery_level);
  else if (StringIsEqual(name, "noncomp_vario"))
    Lua::PushOptional(L, basic.noncomp_vario_available, basic.noncomp_vario);
  else if (StringIsEqual(name, "total_energy_vario"))
    Lua::PushOptional(L, basic.total_energy_vario_available, basic.total_energy_vario);
  else if (StringIsEqual(name, "netto_vario"))
    Lua::PushOptional(L, basic.netto_vario_available, basic.netto_vario);
  else
    return 0;

  return 1;
}

void
Lua::InitBlackboard(lua_State *L)
{
  lua_getglobal(L, "xcsoar");

  lua_newtable(L);

  MakeIndexMetaTableFor(L, RelativeStackIndex{-1}, l_blackboard_index);

  lua_setfield(L, -2, "blackboard");

  lua_pop(L, 1);
}
