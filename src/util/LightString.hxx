// SPDX-License-Identifier: BSD-2-Clause
// author: Max Kellermann <max.kellermann@gmail.com>

#pragma once

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
	BasicAllocatedString<T> allocation;

	explicit LightString(pointer _allocation) noexcept
		:StringPointer<T>(_allocation),
		allocation(BasicAllocatedString<T>::Donate(_allocation)) {}

public:
	explicit LightString(const_pointer _value) noexcept
		:StringPointer<T>(_value), allocation(nullptr) {}

	LightString(std::nullptr_t n) noexcept
		:StringPointer<T>(n), allocation(n) {}

	LightString(LightString &&src) noexcept
		:StringPointer<T>(src),
		 allocation(std::move(src.allocation)) {}

	LightString(BasicAllocatedString<T> &&src) noexcept
		:StringPointer<T>(src.c_str()),
		 allocation(std::move(src)) {}

	static LightString Donate(pointer allocation) noexcept {
		return LightString(allocation);
	}

	static LightString Null() noexcept {
		return nullptr;
	}

	LightString &operator=(LightString &&src) noexcept {
		*(StringPointer<T> *)this = src;
		allocation = std::move(src.allocation);
		return *this;
	}

	pointer Steal() noexcept {
		return allocation.Steal();
	}
};
