// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The XCSoar Project

#pragma once

struct lua_State;

namespace Lua {

/**
 * Provides the Lua table "xcsoar.task".
 */
void
InitTask(lua_State *L);

}
