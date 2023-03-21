// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The XCSoar Project

#include "Dialogs.hpp"
#include "Catch.hpp"
#include "Error.hxx"
#include "util/ConvertString.hpp"
#include "Dialogs/Message.hpp"
#include "Dialogs/Error.hpp"

extern "C" {
#include <lua.h>
}

static int
l_alert(lua_State *L)
{
  const char *message = lua_tostring(L, 1);
  if (message != nullptr) {
    const UTF8ToWideConverter c_message(message);
    if (c_message.IsValid())
      ShowMessageBox(c_message, _T("Lua"), MB_OK|MB_ICONINFORMATION);
  }

  return 0;
}

static void
DialogCatchCallback(Lua::Error &&error)
{
  ShowError(std::make_exception_ptr(std::move(error)), _T("Lua"));
}

void
Lua::InitDialogs(lua_State *L)
{
  lua_register(L, "alert", l_alert);

  SetCatchCallback(L, DialogCatchCallback);
}
