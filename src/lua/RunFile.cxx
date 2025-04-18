// SPDX-License-Identifier: BSD-2-Clause
// author: Max Kellermann <max.kellermann@gmail.com>

#include "RunFile.hxx"
#include "Error.hxx"
#include "system/ConvertPathName.hpp"
#include "system/Path.hpp"

extern "C" {
#include <lauxlib.h>
}

void
Lua::RunFile(lua_State *L, Path path)
{
	if (luaL_loadfile(L, NarrowPathName(path)) || lua_pcall(L, 0, 0, 0))
		throw PopError(L);
}
