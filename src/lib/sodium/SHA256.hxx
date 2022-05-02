/*
 * Copyright 2020-2022 Max Kellermann <max.kellermann@gmail.com>
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

#include <sodium/crypto_hash_sha256.h>

#include <array>
#include <span>
#include <string_view>

using SHA256Digest = std::array<std::byte, crypto_hash_sha256_BYTES>;

class SHA256State {
	crypto_hash_sha256_state state;

public:
	SHA256State() noexcept {
		crypto_hash_sha256_init(&state);
	}

	void Update(std::span<const std::byte> p) noexcept {
		crypto_hash_sha256_update(&state,
					  (const unsigned char *)p.data(),
					  p.size());
	}

	template<typename T>
	void Update(std::span<const T> src) noexcept {
		Update(std::as_bytes(src));
	}

	void Update(std::string_view src) noexcept {
		Update(std::span<const char>{src});
	}

	template<typename T>
	void UpdateT(const T &p) noexcept {
		const std::span<const T> span{&p, 1};
		Update(span);
	}

	void Final(void *out) noexcept {
		crypto_hash_sha256_final(&state, (unsigned char *)out);
	}

	auto Final() noexcept {
		SHA256Digest out;
		Final(&out);
		return out;
	}
};

[[gnu::pure]]
inline auto
SHA256(std::span<const std::byte> src) noexcept
{
	SHA256Digest out;
	crypto_hash_sha256(reinterpret_cast<unsigned char *>(out.data()),
			   (const unsigned char *)src.data(),
			   src.size());
	return out;
}

template<typename T>
[[gnu::pure]]
inline auto
SHA256(std::span<const T> src) noexcept
{
	return SHA256(std::as_bytes(src));
}

[[gnu::pure]]
inline auto
SHA256(std::string_view src) noexcept
{
	return SHA256(std::span<const char>{src});
}
