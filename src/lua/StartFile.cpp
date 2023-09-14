// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The XCSoar Project

#include "StartFile.hpp"
#include "RunFile.hxx"
#include "Full.hpp"
#include "Persistent.hpp"
#include "Background.hpp"
#include "system/Path.hpp"

extern "C" {
#include <lua.h>
}

void
Lua::StartFile(Path path)
{
  StatePtr state(Lua::NewFullState());
  RunFile(state.get(), path);

  if (IsPersistent(state.get()))
    AddBackground(std::move(state));
}
