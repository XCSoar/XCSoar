// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The XCSoar Project

#pragma once

struct lua_State;

namespace Lua {

/**
 * Wrapper for luaL_newstate() that initialises all XCSoar APIs.
 */
lua_State *
NewFullState();

}
