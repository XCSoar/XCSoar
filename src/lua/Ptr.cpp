// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The XCSoar Project

#include "Ptr.hpp"

extern "C" {
#include <lua.h>
}

void
Lua::StateDeleter::operator()(lua_State *state) const
{
  lua_close(state);
}
