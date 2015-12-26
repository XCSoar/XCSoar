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

#include "Map.hpp"
#include "Geo.hpp"
#include "Util.hpp"
#include "Util/StringAPI.hxx"
#include "UIGlobals.hpp"
#include "PageActions.hpp"
#include "MapWindow/GlueMapWindow.hpp"
#include "Pan.hpp"

extern "C" {
#include <lauxlib.h>
}

static int
l_map_index(lua_State *L)
{
  const char *name = lua_tostring(L, 2);
  if (name == nullptr)
    return 0;

  auto *map = UIGlobals::GetMap();
  if (map == nullptr)
    return 0;

  if (StringIsEqual(name, "location"))
    Lua::Push(L, map->GetLocation());
  else if (StringIsEqual(name, "is_panning"))
    Lua::Push(L, IsPanning());
  else
    return 0;

  return 1;
}

static int
l_map_show(lua_State *L)
{
  if (lua_gettop(L) != 0)
    return luaL_error(L, "Invalid parameters");

  PageActions::ShowMap();
  return 0;
}

static int
l_map_enterpan(lua_State *L)
{
  if (lua_gettop(L) != 0)
    return luaL_error(L, "Invalid parameters");

  EnterPan();
  return 0;
}

static int
l_map_leavepan(lua_State *L)
{
  if (lua_gettop(L) != 0)
    return luaL_error(L, "Invalid parameters");

  LeavePan();
  return 0;
}

static int
l_map_disablepan(lua_State *L)
{
  if (lua_gettop(L) != 0)
    return luaL_error(L, "Invalid parameters");
  /**
   * Low-level version of LeavePan().  It disables panning in the map
   * and updates the input mode, but does not restore the page layout.
   * Only to be used by the pages library.
   */
  DisablePan();
  return 0;
}

static int
l_map_panto(lua_State *L)
{
  if (lua_gettop(L) != 2)
    return luaL_error(L, "Invalid parameters");

  GeoPoint location = GeoPoint(Angle::Degrees(luaL_checknumber(L, 2)),
                  Angle::Degrees(luaL_checknumber(L, 1)));
 
  PanTo(location);
  return 0;
}

static int
l_map_pancursor(lua_State *L)
{
  if (lua_gettop(L) != 2)
    return luaL_error(L, "Invalid parameters");

  int dx = luaL_checknumber(L, 1);
  int dy = luaL_checknumber(L, 2);
  GlueMapWindow *map_window = UIGlobals::GetMapIfActive();
  if (map_window == NULL || !map_window->IsPanning())
    return 0;

  const WindowProjection &projection = map_window->VisibleProjection();
  if (!projection.IsValid())
    return 0;

  RasterPoint pt = projection.GetScreenOrigin();
  pt.x -= dx * int(projection.GetScreenWidth()) / 4;
  pt.y -= dy * int(projection.GetScreenHeight()) / 4;
  map_window->SetLocation(projection.ScreenToGeo(pt));

  map_window->QuickRedraw();
  return 0;
}

static constexpr struct luaL_Reg map_funcs[] = {
  {"show", l_map_show},
  {"enterpan", l_map_enterpan},
  {"leavepan", l_map_leavepan},
  {"disablepan", l_map_disablepan},
  {"panto", l_map_panto},
  {"pancursor", l_map_pancursor},
  {nullptr, nullptr}
};

void
Lua::InitMap(lua_State *L)
{
  lua_getglobal(L, "xcsoar");

  lua_newtable(L);

  lua_newtable(L);
  SetField(L, -2, "__index", l_map_index);
  lua_setmetatable(L, -2);

  luaL_setfuncs(L, map_funcs, 0);

  lua_setfield(L, -2, "map");

  lua_pop(L, 1);
}
