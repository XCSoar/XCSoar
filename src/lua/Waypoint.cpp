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

#include "Waypoint.hpp"
#include "Dialogs/Waypoint/WaypointDialogs.hpp"

#include "MetaTable.hxx"
#include "UIGlobals.hpp"
#include "Message.hpp"
#include "Task/ProtectedTaskManager.hpp"
#include "LogFile.hpp"
#include "Components.hpp"
#include "Interface.hpp"

extern "C" {
#include <lauxlib.h>
}

static int
l_waypoint_index(lua_State *L)
{
  /*
   const char *name = lua_tostring(L, 2);
   if (name == nullptr)
   return 0;

   if (StringIsEqual(name, "location"))
   Lua::Push(L, waypoint->GetLocation());
   else if (StringIsEqual(name, "is_panning"))
   Lua::Push(L, IsPanning());
   else
   return 0;

   */
  return 1;
}

static int
l_waypoint_showCurrentWaypoint(lua_State *L)
{
  if (protected_task_manager == NULL)
    return 0;
  WaypointPtr wp;

  wp = protected_task_manager->GetActiveWaypoint();
  if (!wp) {
    Message::AddMessage(_T("No active waypoint!"));
    return 1;
  }

  /* due to a code limitation, we can't currently manipulate
   Waypoint instances taken from the task, because it would
   require updating lots of internal task state, and the waypoint
   editor doesn't know how to do that */
  bool allow_navigation = false;
  bool allow_edit = false;
  if (wp)
    dlgWaypointDetailsShowModal(std::move(wp), allow_navigation, allow_edit);

  return 0;
}

static int
l_waypoint_showSelectedWaypoint(lua_State *L)
{
  bool allow_navigation = true;
  bool allow_edit = true;
  if (protected_task_manager == NULL)
    return 0;
  WaypointPtr wp;
  const NMEAInfo &basic = CommonInterface::Basic();
  wp = ShowWaypointListDialog(basic.location);
  if (wp)
    dlgWaypointDetailsShowModal(std::move(wp), allow_navigation, allow_edit);
  return 0;
}

static constexpr struct luaL_Reg waypoint_funcs[] = {
  { "showCurrent", l_waypoint_showCurrentWaypoint},
  { "showSelected", l_waypoint_showSelectedWaypoint},
  { nullptr, nullptr}
};

void
Lua::InitWaypoint(lua_State *L)
{
  lua_getglobal(L, "xcsoar");

  lua_newtable(L);

  MakeIndexMetaTableFor(L, RelativeStackIndex { -1 }, l_waypoint_index);

  luaL_setfuncs(L, waypoint_funcs, 0);

  lua_setfield(L, -2, "waypoint");

  lua_pop(L, 1);
}
