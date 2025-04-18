// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The XCSoar Project

#pragma once

struct lua_State;

namespace Lua {

struct RelativeStackIndex;

void
InitAssociatePointer(lua_State *L, const char *table);

/**
 * Associate the given pointer with a native Lua value.
 */
void
AssociatePointer(lua_State *L, const char *table,
                 void *p, RelativeStackIndex value_idx);

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
[[gnu::pure]]
bool
HasPointerAssociations(lua_State *L, const char *table,
                       bool (*predicate)(void *key)=nullptr);

}
