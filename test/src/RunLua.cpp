/*
Copyright_License {

  XCSoar Glide Computer - http://www.xcsoar.org/
  Copyright (C) 2000-2021 The XCSoar Project
  A detailed list of copyright holders can be found in the file "AUTHORS".

  This program is free software; you can redistribute it and/or
  modify it under the terms of the GNU General Public License
  as published by the Free Software Foundation; either version 2
  of the License, or (at your option) any later version.

  This program is distributed in the hope that it will be useful,
  but WITHOUT ANY WARRANTY; without even the implied warranty of
  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
  GNU General Public License for more details.

  You should have received a copy of the GNU General Public License
  along with this program; if not, write to the Free Software
  Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA  02111-1307, USA.
}
*/

#include "lua/Basic.hpp"
#include "lua/Log.hpp"
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

  lua_register(state.get(), "alert", l_alert);

  Lua::RunFile(state.get(), path);

  return EXIT_SUCCESS;
} catch (...) {
  PrintException(std::current_exception());
  return EXIT_FAILURE;
}
