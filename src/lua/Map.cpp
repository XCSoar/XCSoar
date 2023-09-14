// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The XCSoar Project

#include "Map.hpp"
#include "Geo.hpp"
#include "MetaTable.hxx"
#include "Util.hxx"
#include "util/StringAPI.hxx"
#include "UIGlobals.hpp"
#include "PageActions.hpp"
#include "MapWindow/GlueMapWindow.hpp"
#include "Pan.hpp"
#include "Language/Language.hpp"
#include "Message.hpp"
#include "Interface.hpp"
#include "Profile/Profile.hpp"
#include "Profile/Keys.hpp"
#include "Math/Constants.hpp"

extern "C" {
#include <lauxlib.h>
}

#include <algorithm> // for std::clamp()

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
  const auto size = projection.GetScreenSize();
  pt.x -= dx * int(size.width) / 4;
  pt.y -= dy * int(size.height) / 4;
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

  value = std::clamp(value, minreasonable, scale_1600km);
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

  MakeIndexMetaTableFor(L, RelativeStackIndex{-1}, l_map_index);

  luaL_setfuncs(L, map_funcs, 0);

  lua_setfield(L, -2, "map");

  lua_pop(L, 1);
}
