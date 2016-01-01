/*
Copyright_License {

  XCSoar Glide Computer - http://www.xcsoar.org/
  Copyright (C) 2000-2016 The XCSoar Project
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

#include "Log.hpp"
#include "LogFile.hpp"

extern "C" {
#include <lauxlib.h>
}

#include <string>
#include <algorithm>

static int
l_print(lua_State *L)
{
  std::string message;

  const unsigned n = lua_gettop(L);
  lua_getglobal(L, "tostring");
  for (unsigned i = 1; i <= n; ++i) {
    lua_pushvalue(L, -1);
    lua_pushvalue(L, i);
    lua_call(L, 1, 1);

    size_t l;
    const char *s = lua_tolstring(L, -1, &l);  /* get result */
    if (s == nullptr)
      return luaL_error(L, "'tostring' must return a string to 'print'");
    if (i > 1)
      message.push_back(' ');
    message.append(s, l);
    lua_pop(L, 1);
  }

  std::replace_if(message.begin(), message.end(),
                  [](unsigned char ch){ return ch < ' '; }, ' ');
  LogFormat("%s", message.c_str());
  return 0;
}

void
Lua::InitLog(lua_State *L)
{
  lua_register(L, "print", l_print);
}
