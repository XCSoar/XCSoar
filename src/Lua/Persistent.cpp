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

#include "Persistent.hpp"
#include "Util.hxx"

extern "C" {
#include <lua.h>
}

static constexpr char persistent_table[] = "xcsoar.persistent_table";
static constexpr char persistent_callback[] = "xcsoar.persistent_callback";

void
Lua::InitPersistent(lua_State *L)
{
  lua_newtable(L);
  lua_setfield(L, LUA_REGISTRYINDEX, persistent_table);
}

void
Lua::SetPersistentCallback(lua_State *L, PersistentCallback callback)
{
  Lua::SetRegistry(L, persistent_callback,
                   Lua::LightUserData((void *)callback));
}

gcc_pure
static Lua::PersistentCallback
GetPersistentCallback(lua_State *L)
{
  return (Lua::PersistentCallback)
    Lua::GetRegistryLightUserData(L, persistent_callback);
}

bool
Lua::IsPersistent(lua_State *L)
{
  bool result = false;

  lua_getfield(L, LUA_REGISTRYINDEX, persistent_table);
  if (!lua_isnil(L, -1)) {
    lua_pushnil(L);
    if (lua_next(L, -2)) {
      lua_pop(L, 2); // pop key, value
      result = true;
    }
  }

  lua_pop(L, 1); // pop table
  return result;
}

void
Lua::AddPersistent(lua_State *L, void *p)
{
  lua_getfield(L, LUA_REGISTRYINDEX, persistent_table);
  if (!lua_isnil(L, -1)) {
    SetTable(L, -3, LightUserData(p), true);
  }

  lua_pop(L, 1); // pop table
}

void
Lua::RemovePersistent(lua_State *L, void *p)
{
  lua_getfield(L, LUA_REGISTRYINDEX, persistent_table);

  if (!lua_isnil(L, -1)) {
    SetTable(L, -3, LightUserData(p), nullptr);
  }

  lua_pop(L, 1); // pop table
}

void
Lua::CheckPersistent(lua_State *L)
{
  auto callback = GetPersistentCallback(L);
  if (callback == nullptr)
    return;

  lua_getfield(L, LUA_REGISTRYINDEX, persistent_table);
  if (lua_isnil(L, -1)) {
    lua_pop(L, 1); // pop nil
    return;
  }

  lua_pushnil(L);
  if (lua_next(L, -2)) {
    lua_pop(L, 3); // pop value, key, table
    return;
  }

  callback(L);
}
