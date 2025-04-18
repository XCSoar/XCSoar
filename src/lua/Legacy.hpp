// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The XCSoar Project

#pragma once

struct lua_State;

namespace Lua {

/**
 * Provide legacy APIs, e.g. a function that calls an old-style
 * InputEvent.
 */
void
InitLegacy(lua_State *L);

}
