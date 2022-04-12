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
