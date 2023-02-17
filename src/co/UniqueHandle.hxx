// SPDX-License-Identifier: BSD-2-Clause
// Copyright CM4all GmbH
// author: Max Kellermann <mk@cm4all.com>

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

	auto release() noexcept {
		return std::exchange(value, nullptr);
	}
};

} // namespace Co
