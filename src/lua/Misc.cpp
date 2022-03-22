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

#include "Misc.hpp"
#include "MainWindow.hpp"
#include "MetaTable.hxx"
#include "Util.hxx"
#include "util/StringAPI.hxx"
#include "UIGlobals.hpp"
#include "Components.hpp"
#include "Dialogs/dlgAnalysis.hpp"
#include "Dialogs/Dialogs.h"
#include "Language/Language.hpp"
#include "Message.hpp"
#include "Interface.hpp"
#include "Profile/Profile.hpp"
#include "Profile/ProfileKeys.hpp"
#include "Math/Constants.hpp"
#include "util/Clamp.hpp"
#include "LogFile.hpp"
extern "C" {
#include <lauxlib.h>
}

static int
l_misc_index(lua_State *L)
{
  /*
  const char *name = lua_tostring(L, 2);
  if (name == nullptr)
    return 0;

  if (StringIsEqual(name, "location"))
    Lua::Push(L, misc->GetLocation());
  else if (StringIsEqual(name, "is_panning"))
    Lua::Push(L, IsPanning());
  else
    return 0;

  */
  return 1;
}



static int
l_misc_analysis(lua_State *L)
{
  dlgAnalysisShowModal(*CommonInterface::main_window,
                       CommonInterface::main_window->GetLook(),
                       CommonInterface::Full(),
                       *glide_computer,
                       &airspace_database,
                       terrain);
  return 0;
}
static int
l_misc_checklist(lua_State *L)
{
  dlgChecklistShowModal();

  return 0;
}


static constexpr struct luaL_Reg misc_funcs[] = {
  {"analysis", l_misc_analysis},
  {"checklist", l_misc_checklist},
  {nullptr, nullptr}
};

void
Lua::InitMisc(lua_State *L)
{
  lua_getglobal(L, "xcsoar");

  lua_newtable(L);

  MakeIndexMetaTableFor(L, RelativeStackIndex{-1}, l_misc_index);

  luaL_setfuncs(L, misc_funcs, 0);

  lua_setfield(L, -2, "misc");

  lua_pop(L, 1);
}
