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

#include "Map.hpp"
#include "Geo.hpp"
#include "Util.hxx"
#include "Util/StringAPI.hxx"
#include "UIGlobals.hpp"
#include "PageActions.hpp"
#include "MapWindow/GlueMapWindow.hpp"
#include "Pan.hpp"
#include "Language/Language.hpp"
#include "Message.hpp"
#include "Interface.hpp"
#include "Profile/Profile.hpp"
#include "Profile/ProfileKeys.hpp"
#include "Util/Clamp.hpp"


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

  auto pt = projection.GetScreenOrigin();
  pt.x -= dx * int(projection.GetScreenWidth()) / 4;
  pt.y -= dy * int(projection.GetScreenHeight()) / 4;
  map_window->SetLocation(projection.ScreenToGeo(pt));

  map_window->QuickRedraw();
  return 0;
}

static int
l_map_zoom(lua_State *L)
{
  if (lua_gettop(L) != 1)
    return luaL_error(L, "Invalid parameters");

  int factor = luaL_checknumber(L, 1);
  GlueMapWindow *map_window = PageActions::ShowMap();
  if (map_window == NULL)
    return 0;

  const MapWindowProjection &projection =
      map_window->VisibleProjection();
  if (!projection.IsValid())
    return 0;

  auto value = projection.GetMapScale();

  if (projection.HaveScaleList()) {
    value = projection.StepMapScale(value, -factor);
  } else {
    if (factor == 1)
      // zoom in a little
      value /= M_SQRT2;
    else if (factor == -1)
      // zoom out a little
      value *= M_SQRT2;
    else if (factor == 2)
      // zoom in a lot
      value /= 2;
    else if (factor == -2)
      // zoom out a lot
      value *= 2;
  }

  MapSettings &settings_map = CommonInterface::SetMapSettings();
  if (map_window == NULL)
    return 0;

  const DisplayMode displayMode = CommonInterface::GetUIState().display_mode;
  if (settings_map.auto_zoom_enabled &&
      !(displayMode == DisplayMode::CIRCLING && settings_map.circle_zoom_enabled) &&
      !IsPanning()) {
    settings_map.auto_zoom_enabled = false;  // disable autozoom if user manually changes zoom
    Profile::Set(ProfileKeys::AutoZoom, false);
    Message::AddMessage(_("Auto. zoom off"));
  }
 
  auto vmin = CommonInterface::GetComputerSettings().polar.glide_polar_task.GetVMin();
  auto scale_2min_distance = vmin * 12;
  constexpr double scale_100m = 10;
  constexpr double scale_1600km = 1600 * 100;
  auto minreasonable = displayMode == DisplayMode::CIRCLING
    ? scale_100m
    : std::max(scale_100m, scale_2min_distance);

  value = Clamp(value, minreasonable, scale_1600km);
  map_window->SetMapScale(value);
  map_window->QuickRedraw();

  return 0;
}

static int
l_map_next(lua_State *L)
{
  if (lua_gettop(L) != 0)
    return luaL_error(L, "Invalid parameters");

  PageActions::Next();
  return 0;
}

static int
l_map_prev(lua_State *L)
{
  if (lua_gettop(L) != 0)
    return luaL_error(L, "Invalid parameters");

  PageActions::Prev();
  return 0;
}

static constexpr struct luaL_Reg map_funcs[] = {
  {"show", l_map_show},
  {"enterpan", l_map_enterpan},
  {"leavepan", l_map_leavepan},
  {"disablepan", l_map_disablepan},
  {"panto", l_map_panto},
  {"pancursor", l_map_pancursor},
  {"zoom", l_map_zoom},
  {"next", l_map_next},
  {"prev", l_map_prev},
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
