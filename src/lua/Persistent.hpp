// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The XCSoar Project

#pragma once

struct lua_State;

namespace Lua {

typedef void (*PersistentCallback)(lua_State *L);

void
InitPersistent(lua_State *L);

void
SetPersistentCallback(lua_State *L, PersistentCallback callback);

bool
IsPersistent(lua_State *L);

void
AddPersistent(lua_State *L, void *p);

void
RemovePersistent(lua_State *L, void *p);

void
CheckPersistent(lua_State *L);

}
