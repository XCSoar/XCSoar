// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The XCSoar Project

#include "Radio.hpp"
#include "Dialogs/FrequencyBrowserDialog.hpp"

extern "C" {
#include <lua.h>
#include <lauxlib.h>
}

static int
l_show_frequency_browser(lua_State *L)
{
  const char *tab = nullptr;
  if (lua_gettop(L) >= 1 && lua_isstring(L, 1))
    tab = lua_tostring(L, 1);

  ShowFrequencyBrowserDialog(tab);
  return 0;
}

void
Lua::InitRadio(lua_State *L) noexcept
{
  lua_getglobal(L, "xcsoar");

  lua_newtable(L);

  lua_pushcfunction(L, l_show_frequency_browser);
  lua_setfield(L, -2, "show_frequency_browser");

  lua_setfield(L, -2, "radio");

  lua_pop(L, 1);
}
