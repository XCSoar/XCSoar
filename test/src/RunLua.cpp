// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The XCSoar Project

#include "lua/Basic.hpp"
#include "lua/Log.hpp"
#include "lua/Http.hpp"
#include "lua/Geo.hpp"
#include "lua/RunFile.hxx"
#include "lua/Ptr.hpp"
#include "system/Args.hpp"
#include "util/PrintException.hxx"

extern "C" {
#include <lua.h>
}

#include <stdio.h>

static int
l_alert(lua_State *L)
{
  fprintf(stderr, "%s\n", lua_tostring(L, 1));
  return 0;
}

int main(int argc, char **argv)
try {
  Args args(argc, argv, "FILE.lua");
  const auto path = args.ExpectNextPath();
  args.ExpectEnd();

  Lua::StatePtr state(Lua::NewBasicState());
  Lua::InitLog(state.get());
  Lua::InitHttp(state.get());
  Lua::InitGeo(state.get());

  lua_register(state.get(), "alert", l_alert);

  Lua::RunFile(state.get(), path);

  return EXIT_SUCCESS;
} catch (...) {
  PrintException(std::current_exception());
  return EXIT_FAILURE;
}
