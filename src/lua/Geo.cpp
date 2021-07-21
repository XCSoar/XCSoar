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

#include "Geo.hpp"
#include "Class.hxx"
#include "Util.hxx"
#include "Geo/GeoPoint.hpp"
#include "Formatter/GeoPointFormatter.hpp"
#include "util/ConvertString.hpp"
#include "util/StringAPI.hxx"

namespace Lua {

static constexpr char lua_geo_point_class[] = "xcsoar.GeoPoint";
using LuaGeoPointClass = Lua::Class<GeoPoint, lua_geo_point_class>;

static int
NewGeoPoint(lua_State *L)
{
  if (lua_gettop(L) != 3)
    return luaL_error(L, "Invalid parameters");

  if (!lua_isnumber(L, 2))
    luaL_argerror(L, 2, "Longitude expected");

  const Angle longitude = ToAngle(L, 2);
  if (longitude < -Angle::HalfCircle() || longitude > Angle::HalfCircle())
    luaL_argerror(L, 2, "Longitude out of range");

  if (!lua_isnumber(L, 3))
    luaL_argerror(L, 3, "Latitude expected");

  const Angle latitude = ToAngle(L, 3);
  if (latitude < -Angle::QuarterCircle() || latitude > Angle::QuarterCircle())
    luaL_argerror(L, 2, "Latitude out of range");

  LuaGeoPointClass::New(L, longitude, latitude);
  return 1;
}

static int
GeoPointIndex(lua_State *L)
{
  if (lua_gettop(L) != 2)
    return luaL_error(L, "Invalid parameters");

  auto &gp = LuaGeoPointClass::Cast(L, 1);

  if (lua_isstring(L, 2)) {
    const char *name = lua_tostring(L, 2);

    if (StringIsEqual(name, "longitude")) {
      Push(L, gp.longitude);
      return 1;
    } else if (StringIsEqual(name, "latitude")) {
      Push(L, gp.latitude);
      return 1;
    }
  }

  return luaL_error(L, "Unknown attribute");
}

static int
GeoPointToString(lua_State *L)
{
  auto &gp = LuaGeoPointClass::Cast(L, 1);
  Lua::Push(L, WideToUTF8Converter(FormatGeoPoint(gp, CoordinateFormat::DDMMSS)));
  return 1;
}

static constexpr struct luaL_Reg geo_point_funcs[] = {
  {"new", NewGeoPoint},
  {nullptr, nullptr}
};

static void
CreateGeoPointMetatable(lua_State *L)
{
  LuaGeoPointClass::Register(L);
  SetTable(L, RelativeStackIndex{-1}, "__index", GeoPointIndex);
  SetTable(L, RelativeStackIndex{-1}, "__tostring", GeoPointToString);
  lua_pop(L, 1);
}

void
InitGeo(lua_State *L) noexcept
{
  const Lua::ScopeCheckStack check_stack(L);

  lua_getglobal(L, "xcsoar");

  luaL_newlib(L, geo_point_funcs);
  lua_setfield(L, -2, "GeoPoint"); // xcsoar.GeoPoint = geo_point_funcs

  lua_pop(L, 1); // pop global "xcsoar"

  CreateGeoPointMetatable(L);
}

void Push(lua_State *L, Angle value) {
  Push(L, value.Degrees());
}

Angle ToAngle(lua_State *L, int idx) {
  return Angle::Degrees(lua_tonumber(L, idx));
}

void Push(lua_State *L, GeoPoint value) {
  if (value.IsValid()) {
    lua_newtable(L);

    LuaGeoPointClass::New(L, value);
  } else
    lua_pushnil(L);
}

GeoPoint ToGeoPoint(lua_State *L, int idx) {
  lua_getfield(L, 1, "longitude");
  lua_getfield(L, 1, "latitude");
  GeoPoint value(ToAngle(L, -2), ToAngle(L, -1));
  lua_pop(L, 2);
  return value;
}

}
