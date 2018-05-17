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

#include "Replay.hpp"
#include "Util.hxx"
#include "OS/Path.hpp"
#include "Components.hpp"
#include "Replay/Replay.hpp"
#include "Util/ConvertString.hpp"

extern "C" {
#include <lauxlib.h>
}

static int
l_replay_index(lua_State *L)
{
  const char *name = lua_tostring(L, 2);
  if (name == nullptr) {
    return 0;
  } else if (StringIsEqual(name, "time_scale")) {
    Lua::Push(L, (int)replay->GetTimeScale());
  } else if (StringIsEqual(name, "virtual_time")) {
    Lua::Push(L, replay->GetVirtualTime());
  } else
    return 0;

  return 1;
}

static int
l_replay_settimescale(lua_State *L)
{

  if (lua_gettop(L) != 1)
    return luaL_error(L, "Invalid parameters");

  float timescale = luaL_checknumber(L, 1);
  replay->SetTimeScale(timescale);

  return 0;
}

static int
l_replay_fastforward(lua_State *L)
{

  if (lua_gettop(L) != 1)
    return luaL_error(L, "Invalid parameters");

  double delta_s = luaL_checknumber(L, 1);
  return !replay->FastForward(delta_s);
}

static int
l_replay_start(lua_State *L)
{
  if (lua_gettop(L) != 1)
    return luaL_error(L, "Invalid parameters");

  const UTF8ToWideConverter filename(luaL_checkstring(L, 1));
  if (filename.IsValid()) {
    Path p(filename);

    try {
      replay->Start(p);
      return 0;
    } catch (const std::runtime_error &) {
    }
  }
  return luaL_error(L, "Replay");
}

static int
l_replay_stop(lua_State *L)
{
  replay->Stop();
  return 0;
}

static constexpr struct luaL_Reg settings_funcs[] = {
  {"set_time_scale", l_replay_settimescale},
  {"fast_forward", l_replay_fastforward},
  {"start", l_replay_start},
  {"stop", l_replay_stop},
  {nullptr, nullptr}
};

void
Lua::InitReplay(lua_State *L)
{
  lua_getglobal(L, "xcsoar");

  lua_newtable(L);

  lua_newtable(L);
  SetField(L, -2, "__index", l_replay_index);
  lua_setmetatable(L, -2);

  luaL_setfuncs(L, settings_funcs, 0);

  lua_setfield(L, -2, "replay");

  lua_pop(L, 1);
}
