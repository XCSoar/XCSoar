// SPDX-License-Identifier: BSD-2-Clause
// author: Max Kellermann <max.kellermann@gmail.com>

#pragma once

#include "StackIndex.hxx"
#include "Util.hxx"

namespace Lua {

/**
 * Create a new meta table with the given "__index" entry and leave it
 * at the top of the stack.
 */
inline void
MakeIndexMetaTable(lua_State *L, lua_CFunction index_function) noexcept
{
	lua_newtable(L);
	SetField(L, RelativeStackIndex{-1}, "__index", index_function);
}

/**
 * Like MakeIndexMetaTable(), but assign it to the given object.
 */
template<typename I>
void
MakeIndexMetaTableFor(lua_State *L, I for_idx,
		      lua_CFunction index_function) noexcept
{
	MakeIndexMetaTable(L, index_function);
	StackPushed(for_idx);
	lua_setmetatable(L, StackIndex{for_idx}.idx);
}

} // namespace Lua
