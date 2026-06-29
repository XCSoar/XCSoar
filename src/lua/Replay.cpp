// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The XCSoar Project

#include "Replay.hpp"
#include "Chrono.hpp"
#include "MetaTable.hxx"
#include "Util.hxx"
#include "system/Path.hpp"
#include "Replay/Replay.hpp"
#include "Components.hpp"
#include "BackendComponents.hpp"
#include "CalculationThread.hpp"
#include "MergeThread.hpp"
#include "Protection.hpp"

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
  } else if (StringIsEqual(name, "is_active")) {
    Lua::Push(L, backend_components->replay->IsActive());
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

  const std::string_view filename(luaL_checkstring(L, 1));
  if (!filename.empty()) {
    Path p(filename.data());

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

static int
l_replay_process_all([[maybe_unused]] lua_State *L)
{
  Replay *replay = backend_components->replay.get();
  if (replay == nullptr || !replay->IsActive()) {
    Lua::Push(L, lua_Integer{0});
    return 1;
  }

  MergeThread *merge_thread = backend_components->merge_thread.get();
  CalculationThread *calc_thread =
    backend_components->calculation_thread.get();
  if (merge_thread == nullptr || calc_thread == nullptr) {
    Lua::Push(L, lua_Integer{0});
    return 1;
  }

  merge_thread->Suspend();

  unsigned count = 0;
  {
    const ScopeSuspendAllThreads suspend;
    count = replay->ProcessAllFixes(*merge_thread, *calc_thread);
  }

  merge_thread->Resume();

  if (count > 0) {
    TriggerCalculatedUpdate();
    TriggerMapUpdate();
  }

  Lua::Push(L, (lua_Integer)count);
  return 1;
}

static constexpr struct luaL_Reg settings_funcs[] = {
  {"set_time_scale", l_replay_settimescale},
  {"fast_forward", l_replay_fastforward},
  {"start", l_replay_start},
  {"stop", l_replay_stop},
  {"process_all", l_replay_process_all},
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
