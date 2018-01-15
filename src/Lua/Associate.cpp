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

#include "Associate.hpp"
#include "Util.hxx"

extern "C" {
#include <lua.h>
}

void
Lua::InitAssociatePointer(lua_State *L, const char *table)
{
  lua_newtable(L);
  lua_setfield(L, LUA_REGISTRYINDEX, table);
}

void
Lua::AssociatePointer(lua_State *L, const char *table,
                      void *p, int value_idx)
{
  lua_getfield(L, LUA_REGISTRYINDEX, table);

  if (value_idx < 0)
    value_idx -= 2;

  SetTable(L, -3, LightUserData(p), StackIndex(value_idx));
  lua_pop(L, 1);
}

void
Lua::DisassociatePointer(lua_State *L, const char *table, void *p)
{
  lua_getfield(L, LUA_REGISTRYINDEX, table);
  SetTable(L, -3, LightUserData(p), nullptr);
  lua_pop(L, 1);
}

void
Lua::LookupPointer(lua_State *L, const char *table, void *p)
{
  lua_getfield(L, LUA_REGISTRYINDEX, table);
  GetTable(L, -2, LightUserData(p));
  lua_remove(L, -2); // pop the registry table
}

bool
Lua::HasPointerAssociations(lua_State *L, const char *table,
                            bool (*predicate)(void *key))
{
  lua_getfield(L, LUA_REGISTRYINDEX, table);

  for (lua_pushnil(L); lua_next(L, -2); lua_pop(L, 1)) {
    if (!lua_isnil(L, -1) && (predicate == nullptr ||
                              predicate(lua_touserdata(L, -2)))) {
      /* pop value, key, table */
      lua_pop(L, 3);
      return true;
    }
  }

  /* pop key, table */
  lua_pop(L, 2);
  return false;
}
