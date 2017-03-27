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

#include "Airspace.hpp"
#include "Util.hxx"
#include "Util/StringAPI.hxx"
#include "Interface.hpp"
#include "Components.hpp"
#include "Engine/Airspace/AbstractAirspace.hpp"
#include "Computer/GlideComputer.hpp"
#include "Airspace/NearestAirspace.hpp"

static int
l_airspace_index(lua_State *L)
{
  const char *name = lua_tostring(L, 2);
  if (name == nullptr)
    return 0;
  else if (StringIsEqual(name, "nearest_vertical_distance")) {
    NearestAirspace nearest = NearestAirspace::FindVertical(CommonInterface::Basic(),
                                                            CommonInterface::Calculated(),
                                                            glide_computer->GetAirspaceWarnings(),
                                                            airspace_database);
    if (!nearest.IsDefined()) return 0;
    Lua::Push(L, nearest.distance);
  } else if (StringIsEqual(name, "nearest_vertical_name")) {
    NearestAirspace nearest = NearestAirspace::FindVertical(CommonInterface::Basic(),
                                                            CommonInterface::Calculated(),
                                                            glide_computer->GetAirspaceWarnings(),
                                                            airspace_database);
    if (!nearest.IsDefined()) return 0;
    Lua::Push(L, nearest.airspace->GetName());
  } else if (StringIsEqual(name, "nearest_horizontal_distance")) {
    NearestAirspace nearest = NearestAirspace::FindHorizontal(CommonInterface::Basic(),
                                                              glide_computer->GetAirspaceWarnings(),
                                                              airspace_database);
    if (!nearest.IsDefined()) return 0;
    Lua::Push(L, nearest.distance);
  } else if (StringIsEqual(name, "nearest_horizontal_name")) {
    NearestAirspace nearest = NearestAirspace::FindHorizontal(CommonInterface::Basic(),
                                                              glide_computer->GetAirspaceWarnings(),
                                                              airspace_database);
    if (!nearest.IsDefined()) return 0;
    Lua::Push(L, nearest.airspace->GetName());
  } else
    return 0;

  return 1;
}

void
Lua::InitAirspace(lua_State *L)
{
  lua_getglobal(L, "xcsoar");

  lua_newtable(L);

  lua_newtable(L);
  SetField(L, -2, "__index", l_airspace_index);
  lua_setmetatable(L, -2);

  lua_setfield(L, -2, "airspace");

  lua_pop(L, 1);
}
