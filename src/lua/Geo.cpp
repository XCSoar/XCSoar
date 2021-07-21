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
#include "Util.hxx"
#include "Geo/GeoPoint.hpp"
#include "Formatter/GeoPointFormatter.hpp"
#include "util/ConvertString.hpp"

extern "C" {
#include <lauxlib.h>
}

static constexpr auto geo_point_meta_table = "xcsoar.GeoPoint";

static int
l_GeoPoint_tostring(lua_State *L)
{
  auto gp = Lua::ToGeoPoint(L, 1);
  Lua::Push(L, WideToUTF8Converter(FormatGeoPoint(gp, CoordinateFormat::DDMMSS)));
  return 1;
}

namespace Lua {

void
InitGeo(lua_State *L) noexcept
{
  luaL_newmetatable(L, geo_point_meta_table);
  SetField(L, RelativeStackIndex{-1}, "__tostring", l_GeoPoint_tostring);
  lua_pop(L, 1);
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

    luaL_getmetatable(L, geo_point_meta_table);
    lua_setmetatable(L, -2);

    SetField(L, RelativeStackIndex{-1}, "longitude", value.longitude);
    SetField(L, RelativeStackIndex{-1}, "latitude", value.latitude);
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
