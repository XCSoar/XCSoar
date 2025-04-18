// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The XCSoar Project

#pragma once

struct lua_State;

namespace Lua {

/**
 * Provide the Lua class "http.request".
 */
void
InitHttp(lua_State *L);

}
