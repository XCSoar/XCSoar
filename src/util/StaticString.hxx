/*
 * Copyright 2010-2021 Max Kellermann <max.kellermann@gmail.com>
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

#ifndef STATIC_STRING_HPP
#define STATIC_STRING_HPP

/** \file */

#include "StringBuffer.hxx"
#include "StringAPI.hxx"
#include "StringUtil.hpp"
#include "StringFormat.hpp"
#include "StringView.hxx"
#include "UTF8.hpp"
#include "ASCII.hxx"

#include <cassert>
#include <cstddef>

#ifdef _UNICODE
#include <wchar.h>
#endif

bool
CopyUTF8(char *dest, size_t dest_size, const char *src);

#ifdef _UNICODE
bool
CopyUTF8(wchar_t *dest, size_t dest_size, const char *src);
#endif

/**
 * A string with a maximum size known at compile time.
 */
template<typename T, size_t max>
class StaticStringBase : public BasicStringBuffer<T, max> {
	typedef BasicStringBuffer<T, max> Base;

public:
	using typename Base::value_type;
	using typename Base::reference;
	using typename Base::pointer;
	using typename Base::const_pointer;
	using typename Base::const_iterator;
	using typename Base::size_type;

	static constexpr value_type SENTINEL = Base::SENTINEL;

	StaticStringBase() = default;
	explicit StaticStringBase(const_pointer value) {
		assign(value);
	}

	using Base::capacity;

	size_type length() const {
		return StringLength(c_str());
	}

	using Base::empty;

	bool full() const {
		return length() >= capacity() - 1;
	}

	/**
	 * Truncate the string to the specified length.
	 *
	 * @param new_length the new length; must be equal or smaller
	 * than the current length
	 */
	void Truncate(size_type new_length) {
		assert(new_length <= length());

		data()[new_length] = SENTINEL;
	}

	void SetASCII(const char *src, const char *src_end) {
		pointer end = ::CopyASCII(data(), capacity() - 1, src, src_end);
		*end = SENTINEL;
	}

	void SetASCII(const char *src) {
		SetASCII(src, src + StringLength(src));
	}

#ifdef _UNICODE
	void SetASCII(const wchar_t *src, const wchar_t *src_end) {
		pointer end = ::CopyASCII(data(), capacity() - 1, src, src_end);
		*end = SENTINEL;
	}

	void SetASCII(const wchar_t *src) {
		SetASCII(src, src + StringLength(src));
	}
#endif

	/**
	 * Eliminate all non-ASCII characters.
	 */
	void CleanASCII() {
		CopyASCII(data(), c_str());
	}

	/**
	 * Copy from the specified UTF-8 string.
	 *
	 * @return false if #src was invalid UTF-8
	 */
	bool SetUTF8(const char *src) {
		return ::CopyUTF8(data(), capacity(), src);
	}

	bool equals(const_pointer other) const {
		assert(other != nullptr);

		return StringIsEqual(c_str(), other);
	}

	[[gnu::pure]]
	bool StartsWith(const_pointer prefix) const {
		return StringStartsWith(c_str(), prefix);
	}

	[[gnu::pure]]
	bool Contains(const_pointer needle) const {
		return StringFind(c_str(), needle) != nullptr;
	}

	using Base::data;

	/**
	 * Returns a writable buffer.
	 */
	pointer buffer() {
		return data();
	}

	/**
	 * Returns one character.  No bounds checking.
	 */
	value_type operator[](size_type i) const {
		assert(i <= length());

		return Base::operator[](i);
	}

	/**
	 * Returns one writable character.  No bounds checking.
	 */
	reference operator[](size_type i) {
		assert(i <= length());

		return Base::operator[](i);
	}

	using Base::begin;

	const_iterator end() const {
		return begin() + length();
	}

	using Base::front;

	value_type back() const {
		return end()[-1];
	}

	void assign(const_pointer new_value) {
		assert(new_value != nullptr);

		CopyString(data(), new_value, capacity());
	}

	void assign(const_pointer new_value, size_type length) {
		assert(new_value != nullptr);

		size_type max_length = length + 1 > capacity()
			? capacity()
			: length + 1;
		CopyString(data(), new_value, max_length);
	}

	void append(const_pointer new_value) {
		assert(new_value != nullptr);

		size_type len = length();
		CopyString(data() + len, new_value, capacity() - len);
	}

	void append(const_pointer new_value, size_type _length) {
		assert(new_value != nullptr);

		size_type len = length();
		size_type max_length = (capacity() - len < _length + 1) ?
			capacity() - len : _length + 1;
		CopyString(data() + len, new_value, max_length);
	}

	bool push_back(value_type ch) {
		size_t l = length();
		if (l >= capacity() - 1)
			return false;

		auto *p = data() + l;
		*p++ = ch;
		*p = SENTINEL;
		return true;
	}

	/**
	 * Append ASCII characters from the specified string without
	 * buffer boundary checks.
	 */
	void UnsafeAppendASCII(const char *p) {
		CopyASCII(data() + length(), p);
	}

	using Base::c_str;

	operator const_pointer() const {
		return c_str();
	}

	operator BasicStringView<T>() const noexcept {
		return c_str();
	}

	bool operator ==(const_pointer value) const {
		return equals(value);
	}

	bool operator !=(const_pointer value) const {
		return !equals(value);
	}

	template<std::size_t other_max>
	[[gnu::pure]]
	bool operator==(const StaticStringBase<T, other_max> &other) const noexcept {
		return *this == other.c_str();
	}

	template<std::size_t other_max>
	[[gnu::pure]]
	bool operator!=(const StaticStringBase<T, other_max> &other) const noexcept {
		return *this != other.c_str();
	}

	StaticStringBase<T, max> &operator =(const_pointer new_value) {
		assign(new_value);
		return *this;
	}

	StaticStringBase<T, max> &operator +=(const_pointer new_value) {
		append(new_value);
		return *this;
	}

	StaticStringBase<T, max> &operator +=(value_type ch) {
		push_back(ch);
		return *this;
	}

	/**
	 * Use snprintf() to set the value of this string.  The value
	 * is truncated if it is too long for the buffer.
	 */
	template<typename... Args>
	BasicStringView<T> Format(const_pointer fmt, Args&&... args) noexcept {
		int s_length = StringFormat(data(), capacity(), fmt, args...);
		if (s_length < 0)
			/* error */
			return nullptr;

		size_type length = (size_type)s_length;
		if (length >= capacity())
			/* truncated */
			length = capacity() - 1;

		return {data(), length};
	}

	/**
	 * Use snprintf() to append to this string.  The value is
	 * truncated if it would become too long for the buffer.
	 */
	template<typename... Args>
	void AppendFormat(const_pointer fmt, Args&&... args) {
		size_t l = length();
		StringFormat(data() + l, capacity() - l, fmt, args...);
	}

	/**
	 * Use sprintf() to set the value of this string.  WARNING:
	 * this does not check if the new value fits into the buffer,
	 * and might overflow.  Use only when you are sure that the
	 * buffer is big enough!
	 */
	template<typename... Args>
	BasicStringView<T> UnsafeFormat(const T *fmt, Args&&... args) noexcept {
		int s_length = StringFormatUnsafe(data(), fmt, args...);
		if (s_length < 0)
			/* error */
			return nullptr;

		size_type length = (size_type)s_length;
		return {data(), length};
	}
};

/**
 * A string with a maximum size known at compile time.
 * This is the char-based sister of the StaticString class.
 */
template<size_t max>
class NarrowString: public StaticStringBase<char, max>
{
	typedef StaticStringBase<char, max> Base;

public:
	using typename Base::value_type;
	using typename Base::reference;
	using typename Base::pointer;
	using typename Base::const_pointer;
	using typename Base::const_iterator;
	using typename Base::size_type;

	NarrowString() = default;
	explicit NarrowString(const_pointer value):Base(value) {}

	NarrowString<max> &operator =(const_pointer new_value) {
		return (NarrowString<max> &)Base::operator =(new_value);
	}

	NarrowString<max> &operator +=(const_pointer new_value) {
		return (NarrowString<max> &)Base::operator +=(new_value);
	}

	NarrowString<max> &operator +=(value_type ch) {
		return (NarrowString<max> &)Base::operator +=(ch);
	}

	void CropIncompleteUTF8() {
		::CropIncompleteUTF8(this->data());
	}
};

#ifdef _UNICODE

/**
 * A string with a maximum size known at compile time.
 * This is the TCHAR-based sister of the NarrowString class.
 */
template<size_t max>
class StaticString: public StaticStringBase<wchar_t, max>
{
	typedef StaticStringBase<wchar_t, max> Base;

public:
	using typename Base::value_type;
	using typename Base::reference;
	using typename Base::pointer;
	using typename Base::const_pointer;
	using typename Base::const_iterator;
	using typename Base::size_type;

	StaticString() = default;
	explicit StaticString(const_pointer value):Base(value) {}

	StaticString<max> &operator =(const_pointer new_value) {
		return (StaticString<max> &)Base::operator =(new_value);
	}

	StaticString<max> &operator +=(const_pointer new_value) {
		return (StaticString<max> &)Base::operator +=(new_value);
	}

	StaticString<max> &operator +=(value_type ch) {
		return (StaticString<max> &)Base::operator +=(ch);
	}

	void CropIncompleteUTF8() {
		/* this is a wchar_t string, it's not multi-byte,
		   therefore we have no incomplete sequences */
	}
};

#else
#define StaticString NarrowString
#endif

#endif
