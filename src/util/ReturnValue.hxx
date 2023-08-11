// SPDX-License-Identifier: BSD-2-Clause
// Copyright CM4all GmbH
// author: Max Kellermann <mk@cm4all.com>

#pragma once

#include <cassert>
#include <concepts>
#include <optional>
#include <utility>

/**
 * Stores the return value of a function.  It does not keep track of
 * whether a value has been set already.
 */
template<typename T>
class ReturnValue {
	std::optional<T> value;

public:
	/**
	 * Set the value.  May be called at most once.
	 */
	template<typename U>
	void Set(U &&_value) noexcept {
		assert(!value);

		value.emplace(std::forward<U>(_value));
	}

	/**
	 * Get (and consume) the value.  May be called at most once,
	 * but only if Set() has been called.
	 */
	[[nodiscard]]
	decltype(auto) Get() && noexcept {
		assert(value);

		return std::move(*value);
	}
};

#if !defined(ANDROID) && !defined(__APPLE__) && (!defined __clang__ || __clang_major__ >=14)

/**
 * Specialization for certain types to eliminate the std::optional
 * overhead.
 */
template<typename T>
requires std::default_initializable<T> && std::movable<T> && std::destructible<T>
class ReturnValue<T> {
	T value;

public:
	template<typename U>
	void Set(U &&_value) noexcept {
		value = std::forward<U>(_value);
	}

	[[nodiscard]]
	T &&Get() && noexcept {
		return std::move(value);
	}
};

#endif

/**
 * This specialization supports returning references.
 */
template<typename T>
class ReturnValue<T &> {
	T *value;

public:
	void Set(T &_value) noexcept {
		value = &_value;
	}

	[[nodiscard]]
	T &Get() && noexcept {
		return *value;
	}
};

template<>
class ReturnValue<void> {
public:
	void Set() noexcept {}
	void Get() && noexcept {}
};
