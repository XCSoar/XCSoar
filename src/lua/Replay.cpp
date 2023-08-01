// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The XCSoar Project

#include "Replay.hpp"
#include "Chrono.hpp"
#include "MetaTable.hxx"
#include "Util.hxx"
#include "system/Path.hpp"
#include "Replay/Replay.hpp"
#include "util/ConvertString.hpp"
#include "Components.hpp"
#include "BackendComponents.hpp"

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
    Lua::Push(L, (lua_Integer)backend_components->replay->GetTimeScale());
  } else if (StringIsEqual(name, "virtual_time")) {
    Lua::Push(L, backend_components->replay->GetVirtualTime());
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
  backend_components->replay->SetTimeScale(timescale);

  return 0;
}

static int
l_replay_fastforward(lua_State *L)
{

  if (lua_gettop(L) != 1)
    return luaL_error(L, "Invalid parameters");

  FloatDuration delta_s{luaL_checknumber(L, 1)};
  return !backend_components->replay->FastForward(delta_s);
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
      backend_components->replay->Start(p);
      return 0;
    } catch (...) {
    }
  }
  return luaL_error(L, "Replay");
}

static int
l_replay_stop([[maybe_unused]] lua_State *L)
{
  backend_components->replay->Stop();
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

  MakeIndexMetaTableFor(L, RelativeStackIndex{-1}, l_replay_index);

  luaL_setfuncs(L, settings_funcs, 0);

  lua_setfield(L, -2, "replay");

  lua_pop(L, 1);
}
