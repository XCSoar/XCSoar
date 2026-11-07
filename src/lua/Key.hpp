// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The XCSoar Project

#pragma once

struct lua_State;

namespace Lua {

/**
 * Provide the Lua function "xcsoar.key.press".
 */
void
InitKey(lua_State *L);

}
