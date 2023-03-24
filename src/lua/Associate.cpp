// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The XCSoar Project

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
                      void *p, RelativeStackIndex value_idx)
{
  lua_getfield(L, LUA_REGISTRYINDEX, table);
  StackPushed(value_idx);

  SetTable(L, RelativeStackIndex{-1}, LightUserData(p), value_idx);
  lua_pop(L, 1);
}

void
Lua::DisassociatePointer(lua_State *L, const char *table, void *p)
{
  lua_getfield(L, LUA_REGISTRYINDEX, table);
  SetTable(L, RelativeStackIndex{-1}, LightUserData(p), nullptr);
  lua_pop(L, 1);
}

void
Lua::LookupPointer(lua_State *L, const char *table, void *p)
{
  lua_getfield(L, LUA_REGISTRYINDEX, table);
  GetTable(L, RelativeStackIndex{-1}, LightUserData(p));
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
