// SPDX-License-Identifier: BSD-2-Clause
// author: Max Kellermann <max.kellermann@gmail.com>

#pragma once

#include <algorithm>
#include <cstddef>
#include <span>

/**
 * Fills a string buffer incrementally by appending more data to the
 * end, truncating the string if the buffer is full.
 */
template<typename T=char>
class BasicStringBuilder {
	using value_type = T;
	using pointer = T *;
	using const_pointer = const T *;
	using size_type = std::size_t;

	pointer p;
	const pointer end;

	static constexpr value_type SENTINEL = '\0';

public:
	constexpr explicit BasicStringBuilder(std::span<value_type> b) noexcept
		:p(b.data()), end(p + b.size()) {}

	constexpr BasicStringBuilder(pointer _p, pointer _end) noexcept
		:p(_p), end(_end) {}

	constexpr BasicStringBuilder(pointer _p, size_type size) noexcept
		:p(_p), end(p + size) {}

	constexpr pointer GetTail() const noexcept {
		return p;
	}

	constexpr size_type GetRemainingSize() const noexcept {
		return end - p;
	}

	constexpr bool IsFull() const noexcept {
		return p >= end - 1;
	}

	std::span<value_type> Write() const noexcept {
		return {p, end};
	}

	void Extend(size_type length) noexcept {
		p += length;
	}

	/**
	 * This class gets thrown when the buffer would overflow by an
	 * operation.  The buffer is then in an undefined state.
	 */
	class Overflow {};

	constexpr bool CanAppend(size_type length) const noexcept {
		return p + length < end;
	}

	void CheckAppend(size_type length) const {
		if (!CanAppend(length))
			throw Overflow();
	}

	void Append(T ch) {
		CheckAppend(1);

		*p++ = ch;
		*p = SENTINEL;
	}

	void Append(const_pointer src);
	void Append(const_pointer src, size_t length);

	void Append(std::span<const T> src) {
		Append(src.data(), src.size());
	}

	void Format(const_pointer fmt, ...);

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

class StringBuilder : public BasicStringBuilder<char> {
public:
	using BasicStringBuilder<char>::BasicStringBuilder;
};

/**
 * Helper function for StringBuilder for when all you need is just
 * concatenate several strings into a buffer.
 */
template<typename T, typename... Args>
static inline const T *
BuildString(std::span<T> buffer, Args&&... args)
{
	static_assert(sizeof...(Args) > 0, "Argument list must be non-empty");

	BasicStringBuilder<T> builder{buffer};
	builder.Append(std::forward<Args>(args)...);
	return buffer.data();
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
