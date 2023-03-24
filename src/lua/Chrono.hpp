// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The XCSoar Project

#pragma once

#include "Util.hxx"
#include "time/FloatDuration.hxx"
#include "time/Stamp.hpp"

struct lua_State;

namespace Lua {

static inline void
Push(lua_State *L, FloatDuration value) noexcept
{
  Push(L, value.count());
}

template<class Rep, class Period>
static inline void
Push(lua_State *L, std::chrono::duration<Rep,Period> value) noexcept
{
  Push(L, std::chrono::duration_cast<FloatDuration>(value));
}

static inline void
Push(lua_State *L, TimeStamp value) noexcept
{
  if (value.IsDefined())
    Push(L, value.ToDuration());
  else
    lua_pushnil(L);
}

} // namespace Lua
