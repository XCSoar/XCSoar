// SPDX-License-Identifier: BSD-2-Clause
// author: Max Kellermann <max.kellermann@gmail.com>

#pragma once

#include <sodium/crypto_hash_sha256.h>

#include <array>
#include <span>
#include <string_view>

using SHA256DigestBuffer = std::array<std::byte, crypto_hash_sha256_BYTES>;
using SHA256DigestView = std::span<const std::byte, crypto_hash_sha256_BYTES>;

class SHA256State {
	crypto_hash_sha256_state state;

public:
	SHA256State() noexcept {
		crypto_hash_sha256_init(&state);
	}

	void Update(std::span<const std::byte> p) noexcept {
		crypto_hash_sha256_update(&state,
					  reinterpret_cast<const unsigned char *>(p.data()),
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

	void Final(std::span<std::byte, crypto_hash_sha256_BYTES> out) noexcept {
		crypto_hash_sha256_final(&state,
					 reinterpret_cast<unsigned char *>(out.data()));
	}

	auto Final() noexcept {
		SHA256DigestBuffer out;
		Final(out);
		return out;
	}
};

[[gnu::pure]]
inline auto
SHA256(std::span<const std::byte> src) noexcept
{
	SHA256DigestBuffer out;
	crypto_hash_sha256(reinterpret_cast<unsigned char *>(out.data()),
			   reinterpret_cast<const unsigned char *>(src.data()),
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
