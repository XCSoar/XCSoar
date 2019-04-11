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
#include "Error.hxx"
#include "Catch.hpp"
#include "Class.hxx"
#include "Persistent.hpp"
#include "Event/Timer.hpp"

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
  explicit LuaTimer(lua_State *L, int callback_idx)
    :callback(L, Lua::StackIndex(callback_idx)), timer(L) {}

  ~LuaTimer() {
    Timer::Cancel();
  }

  lua_State *GetLuaState() {
    return callback.GetState();
  }

  void Schedule(Lua::StackIndex timer_index,
                std::chrono::steady_clock::duration d) noexcept {
    const Lua::ScopeCheckStack check_stack(GetLuaState());

    Lua::AddPersistent(GetLuaState(), this);
    timer.Set(timer_index);
    Timer::Schedule(d);
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

public:
  static int l_new(lua_State *L);
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

static constexpr char lua_timer_class[] = "xcsoar.timer";
typedef Lua::Class<LuaTimer, lua_timer_class> LuaTimerClass;

static constexpr auto
LuaToSteadyDuration(lua_Number n) noexcept
{
  return std::chrono::duration_cast<std::chrono::steady_clock::duration>(std::chrono::duration<double>(n));
}

int
LuaTimer::l_new(lua_State *L)
{
  if (lua_gettop(L) != 2)
    return luaL_error(L, "Invalid parameters");

  if (!lua_isnumber(L, 1))
    luaL_argerror(L, 1, "number expected");

  if (!lua_isfunction(L, 2))
    luaL_argerror(L, 1, "function expected");

  auto *timer = LuaTimerClass::New(L, L, 2);
  timer->Schedule(Lua::StackIndex(-2),
                  LuaToSteadyDuration(lua_tonumber(L, 1)));
  return 1;
}

int
LuaTimer::l_cancel(lua_State *L)
{
  auto &timer = LuaTimerClass::Cast(L, 1);
  timer.Cancel();
  return 0;
}

int
LuaTimer::l_schedule(lua_State *L)
{
  if (lua_gettop(L) != 2)
    return luaL_error(L, "Invalid parameters");

  auto &timer = LuaTimerClass::Cast(L, 1);

  if (!lua_isnumber(L, 2))
    luaL_argerror(L, 2, "number expected");

  timer.Schedule(Lua::StackIndex(1),
                 LuaToSteadyDuration(lua_tonumber(L, 2)));
  return 0;
}

static void
CreateTimerMetatable(lua_State *L)
{
  LuaTimerClass::Register(L);

  /* metatable.__index = timer_methods */
  luaL_newlib(L, timer_methods);
  lua_setfield(L, -2, "__index");

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
}
