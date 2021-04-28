/*
 * Copyright 2013-2021 Max Kellermann <max.kellermann@gmail.com>
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

#ifndef ENUM_BIT_SET_HXX
#define ENUM_BIT_SET_HXX

#include <cstdint>
#include <type_traits>

/**
 * A container that stores each enum value as a bit.
 *
 * @param E an enum type that has one element called "COUNT"
 */
template<typename E>
class EnumBitSet {
	// TODO: change to std::is_scoped_enum_v (C++23)
	static_assert(std::is_enum_v<E>,
		      "Parameter type must be an enum");

	using I = uint_least32_t;

	static constexpr I ToMask(E e) noexcept {
		return I(1) << unsigned(e);
	}

	I mask;

	constexpr EnumBitSet(I _mask) noexcept
		:mask(_mask) {}

public:
	static constexpr unsigned N = unsigned(E::COUNT);
	static_assert(N <= sizeof(I) * 8, "enum is too large");

	constexpr EnumBitSet() noexcept:mask(0) {}

	template<typename... Args>
	constexpr EnumBitSet(Args&&... args) noexcept
		:mask((ToMask(args) | ...)) {}

	constexpr EnumBitSet(const EnumBitSet &) noexcept = default;
	constexpr EnumBitSet(EnumBitSet &) noexcept = default;
	constexpr EnumBitSet(EnumBitSet &&) noexcept = default;

	constexpr EnumBitSet &operator=(const EnumBitSet &) noexcept = default;

	constexpr EnumBitSet operator|(const EnumBitSet other) const noexcept {
		return EnumBitSet(mask | other.mask);
	}

	EnumBitSet &operator|=(const EnumBitSet &other) noexcept {
		mask |= other.mask;
		return *this;
	}

	constexpr EnumBitSet operator&(const EnumBitSet other) const noexcept {
		return EnumBitSet(mask & other.mask);
	}

	EnumBitSet &operator&=(const EnumBitSet &other) noexcept {
		mask &= other.mask;
		return *this;
	}

	constexpr EnumBitSet operator~() const noexcept {
		return EnumBitSet(~mask);
	}

	constexpr bool IsEmpty() const noexcept {
		return mask == 0;
	}

	[[gnu::pure]]
	E UncheckedFirst() const noexcept {
		I i = 1;

		for (unsigned j = 0;; ++j, i <<= 1)
			if (mask & i)
				return E(j);
	}

	void Add(const E e) noexcept {
		mask |= ToMask(e);
	}

	constexpr bool Contains(E e) const noexcept {
		return (mask & ToMask(e)) != 0;
	}

	template<typename O>
	void CopyTo(O o) const noexcept {
		for (unsigned i = 0; i < N; ++i) {
			const E e = E(i);
			if (Contains(e))
				*o++ = e;
		}
	}
};

#endif
