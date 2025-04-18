// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The XCSoar Project

#pragma once

struct lua_State;

namespace Lua {

/**
 * Provide a Lua API to show XCSoar dialogs.
 */
void
InitDialogs(lua_State *L);

}
