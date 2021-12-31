/*
 * Copyright 2021 Max Kellermann <max.kellermann@gmail.com>
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
