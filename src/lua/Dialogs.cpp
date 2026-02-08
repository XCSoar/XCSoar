// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The XCSoar Project

#include "Dialogs.hpp"
#include "Catch.hpp"
#include "Error.hxx"
#include "Dialogs/Message.hpp"
#include "Dialogs/Error.hpp"

#include <utility>

extern "C" {
#include <lua.h>
}

static int
l_alert(lua_State *L)
{
  const char *message = lua_tostring(L, 1);
  if (message != nullptr) {
    ShowMessageBox(message, "Lua", MB_OK|MB_ICONINFORMATION);
  }

  return 0;
}

static void
DialogCatchCallback(Lua::Error &&error)
{
  ShowError(std::make_exception_ptr(std::move(error)), "Lua");
}

void
Lua::InitDialogs(lua_State *L)
{
  lua_register(L, "alert", l_alert);

  SetCatchCallback(L, DialogCatchCallback);
}
