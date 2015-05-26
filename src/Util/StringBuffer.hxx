/*
 * Copyright (C) 2010-2015 Max Kellermann <max@duempel.org>
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

#ifndef STRING_BUFFER_HPP
#define STRING_BUFFER_HPP

#include <stddef.h>

/**
 * A statically allocated string buffer.
 */
template<typename T, size_t _CAPACITY>
class StringBuffer {
public:
	typedef T value_type;
	typedef T &reference;
	typedef T *pointer;
	typedef const T *const_pointer;
	typedef const_pointer const_iterator;
	typedef size_t size_type;

	static constexpr size_type CAPACITY = _CAPACITY;
	static constexpr value_type SENTINEL = '\0';

protected:
	value_type the_data[CAPACITY];

public:
	constexpr size_type capacity() const {
		return CAPACITY;
	}

	constexpr bool empty() const {
		return front() == SENTINEL;
	}

	void clear() {
		the_data[0] = SENTINEL;
	}

	constexpr const_pointer c_str() const {
		return the_data;
	}

	pointer data() {
		return the_data;
	}

	constexpr value_type front() const {
		return c_str()[0];
	}

	constexpr const_iterator begin() const {
		return data();
	}

	constexpr const_iterator end() const {
		return data() + capacity();
	}

	constexpr operator const_pointer() const {
		return c_str();
	}
};

#endif
