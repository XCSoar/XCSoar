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

#ifndef LIGHT_STRING_HXX
#define LIGHT_STRING_HXX

#include "StringPointer.hxx"
#include "AllocatedString.hxx"

#include <utility>

/**
 * A string pointer whose memory may or may not be managed by this
 * class.
 */
template<typename T=char>
class LightString : public StringPointer<T> {
public:
	typedef typename StringPointer<T>::value_type value_type;
	typedef typename StringPointer<T>::pointer pointer;
	typedef typename StringPointer<T>::const_pointer const_pointer;

private:
	AllocatedString<T> allocation;

	explicit LightString(pointer _allocation)
		:StringPointer<T>(_allocation),
		allocation(AllocatedString<T>::Donate(_allocation)) {}

public:
	explicit LightString(const_pointer _value)
		:StringPointer<T>(_value), allocation(nullptr) {}

	LightString(std::nullptr_t n)
		:StringPointer<T>(n), allocation(n) {}

	LightString(LightString &&src)
		:StringPointer<T>(std::move(src)),
		 allocation(std::move(src.allocation)) {}

	static LightString Donate(pointer allocation) {
		return LightString(allocation);
	}

	static LightString Null() {
		return nullptr;
	}

	LightString &operator=(LightString &&src) {
		*(StringPointer<T> *)this = std::move(src);
		allocation = std::move(src.allocation);
		return *this;
	}

	pointer Steal() {
		return allocation.Steal();
	}
};

#endif
