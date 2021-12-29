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

#ifndef LUA_STACK_INDEX_HXX
#define LUA_STACK_INDEX_HXX

namespace Lua {

/**
 * This type represents an index on the Lua stack.  It can be used to
 * select the Push() overload which maps to lua_pushvalue().
 */
struct StackIndex {
	int idx;

	explicit constexpr StackIndex(int _idx) noexcept
		:idx(_idx) {}
};

/**
 * Same as #StackIndex, but is automatically adjusted by this C++
 * wrapper library (by calling StackPushed).
 */
struct RelativeStackIndex : StackIndex {
	using StackIndex::StackIndex;
};

template<typename T>
void
StackPushed(T &, int=1) noexcept
{
}

/**
 * Adjust the #RelativeStackIndex after pushing to the Lua stack.  For
 * all types but #RelativeStackIndex, this is a no-op.
 */
inline void
StackPushed(RelativeStackIndex &idx, int n=1) noexcept
{
	idx.idx -= n;
}

} // namespace Lua

#endif
