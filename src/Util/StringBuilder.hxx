/*
 * Copyright (C) 2015 Max Kellermann <max.kellermann@gmail.com>
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

#ifndef STRING_BUILDER_HXX
#define STRING_BUILDER_HXX

#include "TruncateString.hpp"
#include "StringAPI.hxx"

#include <utility>
#include <algorithm>

#include <stddef.h>

/**
 * Fills a string buffer incrementally by appending more data to the
 * end, truncating the string if the buffer is full.
 */
template<typename T=char>
class StringBuilder {
	typedef T value_type;
	typedef T *pointer;
	typedef const T *const_pointer;
	typedef size_t size_type;

	pointer p;
	const pointer end;

	static constexpr value_type SENTINEL = '\0';

public:
	constexpr StringBuilder(pointer _p, pointer _end):p(_p), end(_end) {}
	constexpr StringBuilder(pointer _p, size_type size)
		:p(_p), end(p + size) {}

	constexpr size_type GetRemainingSize() const {
		return end - p;
	}

	constexpr bool IsFull() const {
		return p >= end - 1;
	}

	void Append(T ch) {
		if (!IsFull())
			*p++ = ch;
	}

	void Append(const_pointer src) {
		p = CopyTruncateString(p, GetRemainingSize(), src);
	}

	void Append(const_pointer src, size_t length) {
		p = std::copy_n(src, std::min(length, GetRemainingSize() - 1),
				p);
	}

	template<typename... Args>
	void Append(T ch, Args&&... args) {
		Append(ch);
		Append(std::forward<Args>(args)...);
	}

	template<typename... Args>
	void Append(const_pointer src, Args&&... args) {
		Append(src);
		Append(std::forward<Args>(args)...);
	}

	template<typename... Args>
	void Append(const_pointer src, size_t length, Args&&... args) {
		Append(src, length);
		Append(std::forward<Args>(args)...);
	}
};

/**
 * Helper function for StringBuilder for when all you need is just
 * concatenate several strings into a buffer.
 */
template<typename T, typename... Args>
static inline const T *
BuildString(T *buffer, size_t size, Args&&... args)
{
	static_assert(sizeof...(Args) > 0, "Argument list must be non-empty");

	StringBuilder<T> builder(buffer, size);
	builder.Append(std::forward<Args>(args)...);
	return buffer;
}

/**
 * Helper function for BuildStringUnsafe().
 */
template<typename T>
static inline void
_UnsafeAppendAll(T *dest)
{
	*dest = '\0';
}

template<typename T, typename... Args>
static inline void
_UnsafeAppendAll(T *dest, const T *first, Args&&... args);

/**
 * Helper function for BuildStringUnsafe().
 */
template<typename T, typename... Args>
static inline void
_UnsafeAppendAll(T *dest, T ch, Args&&... args)
{
	*dest++ = ch;
	_UnsafeAppendAll(dest, std::forward<Args>(args)...);
}

/**
 * Helper function for BuildStringUnsafe().
 */
template<typename T, typename... Args>
static inline void
_UnsafeAppendAll(T *dest, const T *first, Args&&... args)
{
	_UnsafeAppendAll(UnsafeCopyStringP(dest, first),
			 std::forward<Args>(args)...);
}

/**
 * Helper function for BuildStringUnsafe().
 */
template<typename T, typename... Args>
static inline void
_UnsafeAppendAll(T *dest, const T *first, size_t first_length, Args&&... args)
{
	_UnsafeAppendAll(std::copy_n(first, first_length, dest),
			 std::forward<Args>(args)...);
}

/**
 * Like BuildString(), but without buffer overflow checks.  Use this
 * only when you are sure that the input strings fit into the buffer.
 */
template<typename T, typename... Args>
static inline const T *
UnsafeBuildString(T *buffer, Args&&... args)
{
	_UnsafeAppendAll(buffer, std::forward<Args>(args)...);
	return buffer;
}

#endif
