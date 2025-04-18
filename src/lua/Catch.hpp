// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The XCSoar Project

#pragma once

struct lua_State;

namespace Lua {

class Error;

typedef void (*CatchCallback)(Error &&error);

/**
 * Register the function that will be called by ThrowError().  It is
 * responsible for logging or showing the error.
 */
void
SetCatchCallback(lua_State *L, CatchCallback callback);

/**
 * Invoke the CatchCallback.
 */
void
ThrowError(lua_State *L, Error &&error);

}
