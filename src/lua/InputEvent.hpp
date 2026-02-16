// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The XCSoar Project

#pragma once

#include "util/Compiler.h"
#include <tchar.h>

struct lua_State;

namespace Lua {

/**
 * Provide the Lua class "xcsoar.input_event".
 */
void
InitInputEvent(lua_State *L);

bool
FireGlideComputerEvent(unsigned event);

bool
FireNMEAEvent(unsigned event);

bool
FireGesture(const char *gesture);

bool
FireKey(unsigned key);

bool
IsGesture(const char *gesture);

}
