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

#ifndef XCSOAR_LUA_ASSOCIATE_HPP
#define XCSOAR_LUA_ASSOCIATE_HPP

#include "Compiler.h"

struct lua_State;

namespace Lua {

void
InitAssociatePointer(lua_State *L, const char *table);

/**
 * Associate the given pointer with a native Lua value.
 */
void
AssociatePointer(lua_State *L, const char *table,
                 void *p, int value_idx);

/**
 * Remove the given pointer from the registry table.
 */
void
DisassociatePointer(lua_State *L, const char *table, void *p);

/**
 * Look up the native Lua value associated with the given pointer
 * and push it on the stack.
 */
void
LookupPointer(lua_State *L, const char *table, void *p);

/**
 * Check the registry table if there is at least one association.
 */
gcc_pure
bool
HasPointerAssociations(lua_State *L, const char *table,
                       bool (*predicate)(void *key)=nullptr);

}

#endif
