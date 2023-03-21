// SPDX-License-Identifier: BSD-2-Clause
// author: Max Kellermann <max.kellermann@gmail.com>

#pragma once

#include <cassert>
#include <cstdint>
#include <optional>

/**
 * An optional value in the range 0..100.  This is similar to
 * std::optional, but packs everything in one byte.
 */
class OptionalPercent {
	static constexpr uint_least8_t INVALID = ~uint_least8_t{};

	uint_least8_t value;

public:
	constexpr OptionalPercent() noexcept = default;

	constexpr OptionalPercent(std::nullopt_t) noexcept
		:value(INVALID) {}

	constexpr OptionalPercent(uint_least8_t _value) noexcept
		:value(_value) {}

	constexpr operator bool() const noexcept {
		return value != INVALID;
	}

	constexpr uint_least8_t operator*() const noexcept {
		assert(*this);

		return value;
	}

	auto &operator=(uint_least8_t _value) noexcept {
		value = _value;
		return *this;
	}

	constexpr void reset() noexcept {
		value = INVALID;
	}
};
