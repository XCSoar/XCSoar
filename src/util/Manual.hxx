/*
 * Copyright 2013-2022 Max Kellermann <max.kellermann@gmail.com>
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

#pragma once

#include <cassert>
#include <new>
#include <type_traits>
#include <utility>

/**
 * Container for an object that gets constructed and destructed
 * manually.  The object is constructed in-place, and therefore
 * without allocation overhead.  It can be constructed and destructed
 * repeatedly.
 */
template<class T>
class Manual {
	using Storage = std::aligned_storage_t<sizeof(T), alignof(T)>;

	Storage storage;

#ifndef NDEBUG
	bool initialized = false;
#endif

public:
	using value_type = T;
	using reference = T &;
	using const_reference =  const T &;
	using pointer = T *;
	using const_pointer = const T *;

#ifndef NDEBUG
	~Manual() noexcept {
		assert(!initialized);
	}
#endif

	/**
	 * Cast a value reference to the containing Manual instance.
	 */
	static constexpr Manual<T> &Cast(reference value) noexcept {
		return reinterpret_cast<Manual<T> &>(value);
	}

	template<typename... Args>
	void Construct(Args&&... args) {
		assert(!initialized);

		::new(&storage) T(std::forward<Args>(args)...);

#ifndef NDEBUG
		initialized = true;
#endif
	}

	void Destruct() noexcept {
		assert(initialized);

		reference t = Get();
		t.T::~T();

#ifndef NDEBUG
		initialized = false;
#endif
	}

	reference Get() noexcept {
		assert(initialized);

		return *std::launder(reinterpret_cast<pointer>(&storage));
	}

	const_reference Get() const noexcept {
		assert(initialized);

		return *std::launder(reinterpret_cast<const_pointer>(&storage));
	}

	operator reference() noexcept {
		return Get();
	}

	operator const_reference() const noexcept {
		return Get();
	}

	pointer operator->() noexcept {
		return &Get();
	}

	const_pointer operator->() const noexcept {
		return &Get();
	}
};
