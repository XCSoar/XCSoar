// SPDX-License-Identifier: BSD-2-Clause
// author: Max Kellermann <max.kellermann@gmail.com>

#pragma once

#include <type_traits>

/**
 * Check if the specified type is "trivial", but allow a non-trivial
 * default constructor.
 */
template<typename T>
using has_trivial_copy_and_destructor =
	std::integral_constant<bool,
			       std::is_trivially_copy_constructible_v<T> &&
			       std::is_trivially_copy_assignable_v<T> &&
			       std::is_trivially_destructible_v<T>>;

/**
 * Check if the specified type is "trivial" in the non-debug build,
 * but allow a non-trivial default constructor in the debug build.
 * This is needed for types that use #DebugFlag.
 */
#ifdef NDEBUG
template<typename T>
using is_trivial_ndebug = std::is_trivial<T>;
#else
template<typename T>
using is_trivial_ndebug = has_trivial_copy_and_destructor<T>;
#endif
