/*
 * Copyright 2011-2021 Max Kellermann <max.kellermann@gmail.com>
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

#ifndef TYPE_TRAITS_HPP
#define TYPE_TRAITS_HPP

/** \file */

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

#endif
