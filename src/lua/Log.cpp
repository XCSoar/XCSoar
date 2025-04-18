// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The XCSoar Project

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
