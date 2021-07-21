/*
 * Copyright 2015-2021 Max Kellermann <max.kellermann@gmail.com>
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 *
 * - Redistributions of source code must retain the above copyright
 * notice, this list of conditions and the following disclaimer.
 *
 * - Redistributions in binary form must reproduce the above copyright
 * notice, this list of conditions and the following disclaimer in the
 * documentation and/or other materials provided with the
 * distribution.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
 * ``AS IS'' AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
 * LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS
 * FOR A PARTICULAR PURPOSE ARE DISCLAIMED.  IN NO EVENT SHALL THE
 * FOUNDATION OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT,
 * INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES
 * (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR
 * SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT,
 * STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
 * ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED
 * OF THE POSSIBILITY OF SUCH DAMAGE.
 */

#ifndef LUA_META_TABLE_HXX
#define LUA_META_TABLE_HXX

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

#endif
