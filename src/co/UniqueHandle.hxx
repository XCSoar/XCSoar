/*
 * Copyright 2020-2021 CM4all GmbH
 * All rights reserved.
 *
 * author: Max Kellermann <mk@cm4all.com>
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

#include "Compat.hxx"

#include <utility>

namespace Co {

/**
 * Manage a std::coroutine_handle<> which is destroyed by the
 * destructor.
 */
template<typename Promise=void>
class UniqueHandle {
	std::coroutine_handle<Promise> value;

public:
	UniqueHandle() = default;

	explicit constexpr UniqueHandle(std::coroutine_handle<Promise> h) noexcept
		:value(h) {}

	UniqueHandle(UniqueHandle<Promise> &&src) noexcept
		:value(std::exchange(src.value, nullptr))
	{
	}

	~UniqueHandle() noexcept {
		if (value)
			value.destroy();
	}

	auto &operator=(UniqueHandle<Promise> &&src) noexcept {
		using std::swap;
		swap(value, src.value);
		return *this;
	}

	operator bool() const noexcept {
		return (bool)value;
	}

	const auto &get() const noexcept {
		return value;
	}

	const auto *operator->() const noexcept {
		return &value;
	}

#ifdef __clang__
	/* the non-const overload is only needed for clang, because in
	   libc++11, some methods are not "const" */
	auto *operator->() noexcept {
		return &value;
	}
#endif
};

} // namespace Co
