// SPDX-License-Identifier: BSD-2-Clause
// author: Max Kellermann <max.kellermann@gmail.com>

#pragma once

#include <concepts>

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

/**
 * Types satisfying this concept can be used as Lua stack index.
 */
template<class T>
concept AnyStackIndex = std::convertible_to<T, StackIndex> ||
	std::is_same_v<T, int>;

} // namespace Lua
