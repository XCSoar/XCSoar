// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The XCSoar Project

#include "Airspace.hpp"
#include "MetaTable.hxx"
#include "Util.hxx"
#include "util/StringAPI.hxx"
#include "Interface.hpp"
#include "Components.hpp"
#include "Engine/Airspace/AbstractAirspace.hpp"
#include "Airspace/NearestAirspace.hpp"
#include "BackendComponents.hpp"
#include "DataComponents.hpp"

static int
l_airspace_index(lua_State *L)
{
  const auto &airspace_database = *data_components->airspaces;
  const auto *airspace_warnings = backend_components->GetAirspaceWarnings();
  const char *name = lua_tostring(L, 2);
  if (name == nullptr)
    return 0;
  else if (StringIsEqual(name, "nearest_vertical_distance")) {
    NearestAirspace nearest = NearestAirspace::FindVertical(CommonInterface::Basic(),
                                                            CommonInterface::Calculated(),
                                                            airspace_warnings,
                                                            airspace_database);
    if (!nearest.IsDefined()) return 0;
    Lua::Push(L, nearest.distance);
  } else if (StringIsEqual(name, "nearest_vertical_name")) {
    NearestAirspace nearest = NearestAirspace::FindVertical(CommonInterface::Basic(),
                                                            CommonInterface::Calculated(),
                                                            airspace_warnings,
                                                            airspace_database);
    if (!nearest.IsDefined()) return 0;
    Lua::Push(L, nearest.airspace->GetName());
  } else if (StringIsEqual(name, "nearest_horizontal_distance")) {
    NearestAirspace nearest = NearestAirspace::FindHorizontal(CommonInterface::Basic(),
                                                              airspace_warnings,
                                                              airspace_database);
    if (!nearest.IsDefined()) return 0;
    Lua::Push(L, nearest.distance);
  } else if (StringIsEqual(name, "nearest_horizontal_name")) {
    NearestAirspace nearest = NearestAirspace::FindHorizontal(CommonInterface::Basic(),
                                                              airspace_warnings,
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

  MakeIndexMetaTableFor(L, RelativeStackIndex{-1}, l_airspace_index);

  lua_setfield(L, -2, "airspace");

  lua_pop(L, 1);
}
