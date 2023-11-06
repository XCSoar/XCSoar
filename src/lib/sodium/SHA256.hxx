// SPDX-License-Identifier: BSD-2-Clause
// author: Max Kellermann <max.kellermann@gmail.com>

#pragma once

#include <sodium/crypto_hash_sha256.h>

#include <array>
#include <cstddef>
#include <span>

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
