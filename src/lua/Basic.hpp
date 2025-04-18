// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The XCSoar Project

#pragma once

struct lua_State;

namespace Lua {

/**
 * Wrapper for luaL_newstate() that loads some basic Lua standard
 * libraries.
 */
lua_State *
NewBasicState();

}
