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

#include "Timer.hpp"
#include "Assert.hxx"
#include "Value.hxx"
#include "Util.hxx"
#include "Error.hpp"
#include "Catch.hpp"
#include "Associate.hpp"
#include "Persistent.hpp"
#include "Event/Timer.hpp"
#include "Compiler.h"

extern "C" {
#include <lauxlib.h>
}

class LuaTimer final : public Timer {
  Lua::Value callback;

  /**
   * A LightUserData pointing to this instance.  It is only set while
   * the #Timer is scheduled, to avoid holding a reference on an
   * otherwise unused Lua object.
   */
  Lua::Value timer;

public:
  static constexpr const char *registry_table = "xcsoar.timers";

  explicit LuaTimer(lua_State *L, int callback_idx)
    :callback(L, Lua::StackIndex(callback_idx)), timer(L) {
    auto d = (LuaTimer **)lua_newuserdata(L, sizeof(LuaTimer **));
    *d = this;

    luaL_setmetatable(L, "xcsoar.timer");

    Register(callback_idx, -1);

    /* 'this' is left on stack */
  }

  ~LuaTimer() {
    Lua::DisassociatePointer(GetLuaState(), registry_table, (void *)this);
  }

  lua_State *GetLuaState() {
    return callback.GetState();
  }

  void Schedule(Lua::StackIndex timer_index, unsigned ms) {
    const Lua::ScopeCheckStack check_stack(GetLuaState());

    Lua::AddPersistent(GetLuaState(), this);
    timer.Set(timer_index);
    Timer::Schedule(ms);
  }

  void Cancel() {
    const Lua::ScopeCheckStack check_stack(GetLuaState());

    Timer::Cancel();
    timer.Set(nullptr);
    Lua::RemovePersistent(GetLuaState(), this);
  }

protected:
  void OnTimer() override {
    const auto L = GetLuaState();
    const Lua::ScopeCheckStack check_stack(L);

    callback.Push();
    timer.Push();
    if (lua_pcall(L, 1, 0, 0))
      Lua::ThrowError(L, Lua::PopError(L));

    Lua::CheckPersistent(L);
  }

private:
  void Register(int callback_idx, int this_idx) {
    const auto L = GetLuaState();

    lua_newtable(L);
    --this_idx;

    Lua::SetField(L, -2, "timer", Lua::StackIndex(this_idx));

    Lua::AssociatePointer(L, registry_table, (void *)this, -1);
    lua_pop(L, 1); // pop table
  }

  gcc_pure
  static LuaTimer &Check(lua_State *L, int idx) {
    auto d = (LuaTimer **)luaL_checkudata(L, idx, "xcsoar.timer");
    luaL_argcheck(L, d != nullptr, idx, "`xcsoar.timer' expected");
    return **d;
  }

public:
  static int l_new(lua_State *L);

  static int l_gc(lua_State *L) {
    auto &timer = Check(L, 1);
    timer.Cancel();
    delete &timer;
    return 0;
  }

  static int l_cancel(lua_State *L);
  static int l_schedule(lua_State *L);
};

static constexpr struct luaL_Reg timer_funcs[] = {
  {"new", LuaTimer::l_new},
  {nullptr, nullptr}
};

static constexpr struct luaL_Reg timer_methods[] = {
  {"cancel", LuaTimer::l_cancel},
  {"schedule", LuaTimer::l_schedule},
  {nullptr, nullptr}
};

int
LuaTimer::l_new(lua_State *L)
{
  if (lua_gettop(L) != 2)
    return luaL_error(L, "Invalid parameters");

  if (!lua_isnumber(L, 1))
    luaL_argerror(L, 1, "number expected");

  if (!lua_isfunction(L, 2))
    luaL_argerror(L, 1, "function expected");

  auto *timer = new LuaTimer(L, 2);
  timer->Schedule(Lua::StackIndex(-2),
                  unsigned(lua_tonumber(L, 1) * 1000));
  return 1;
}

int
LuaTimer::l_cancel(lua_State *L)
{
  auto &timer = Check(L, 1);
  timer.Cancel();
  return 0;
}

int
LuaTimer::l_schedule(lua_State *L)
{
  if (lua_gettop(L) != 2)
    return luaL_error(L, "Invalid parameters");

  auto &timer = Check(L, 1);

  if (!lua_isnumber(L, 2))
    luaL_argerror(L, 2, "number expected");

  timer.Schedule(Lua::StackIndex(1),
                 unsigned(lua_tonumber(L, 2) * 1000));
  return 0;
}

static void
CreateTimerMetatable(lua_State *L)
{
  luaL_newmetatable(L, "xcsoar.timer");

  /* metatable.__index = timer_methods */
  luaL_newlib(L, timer_methods);
  lua_setfield(L, -2, "__index");

  /* metatable.__gc = l_gc */
  lua_pushcfunction(L, LuaTimer::l_gc);
  lua_setfield(L, -2, "__gc");

  /* pop metatable */
  lua_pop(L, 1);
}

void
Lua::InitTimer(lua_State *L)
{
  const Lua::ScopeCheckStack check_stack(L);

  lua_getglobal(L, "xcsoar");

  luaL_newlib(L, timer_funcs); // create 'timer'
  lua_setfield(L, -2, "timer"); // xcsoar.timer = timer
  lua_pop(L, 1); // pop global "xcsoar"

  CreateTimerMetatable(L);

  Lua::InitAssociatePointer(L, LuaTimer::registry_table);
}
