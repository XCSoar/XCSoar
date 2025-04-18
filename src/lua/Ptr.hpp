// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The XCSoar Project

#pragma once

#include <memory>

struct lua_State;

namespace Lua {

struct StateDeleter {
  void operator()(lua_State *state) const;
};

using StatePtr = std::unique_ptr<lua_State, StateDeleter>;

}
