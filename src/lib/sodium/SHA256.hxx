// SPDX-License-Identifier: BSD-2-Clause
// author: Max Kellermann <max.kellermann@gmail.com>

#pragma once

#if defined(__APPLE__)
#include <CommonCrypto/CommonDigest.h>
#else
#include <sodium/crypto_hash_sha256.h>
#endif

#include <array>
#include <cstddef>
#include <span>

#if defined(__APPLE__)
static constexpr std::size_t SHA256_DIGEST_SIZE = CC_SHA256_DIGEST_LENGTH;
using SHA256DigestBuffer = std::array<std::byte, CC_SHA256_DIGEST_LENGTH>;
using SHA256DigestView = std::span<const std::byte, CC_SHA256_DIGEST_LENGTH>;
#else
static constexpr std::size_t SHA256_DIGEST_SIZE = crypto_hash_sha256_BYTES;
using SHA256DigestBuffer = std::array<std::byte, crypto_hash_sha256_BYTES>;
using SHA256DigestView = std::span<const std::byte, crypto_hash_sha256_BYTES>;
#endif

class SHA256State {
#if defined(__APPLE__)
	CC_SHA256_CTX state;
#else
	crypto_hash_sha256_state state;
#endif

public:
	SHA256State() noexcept {
#if defined(__APPLE__)
		CC_SHA256_Init(&state);
#else
		crypto_hash_sha256_init(&state);
#endif
	}

	void Update(std::span<const std::byte> p) noexcept {
#if defined(__APPLE__)
		CC_SHA256_Update(&state, p.data(), p.size());
#else
		crypto_hash_sha256_update(&state,
					  reinterpret_cast<const unsigned char *>(p.data()),
					  p.size());
#endif
	}

	void Final(std::span<std::byte, SHA256_DIGEST_SIZE> out) noexcept {
#if defined(__APPLE__)
		CC_SHA256_Final(reinterpret_cast<unsigned char *>(out.data()), &state);
#else
		crypto_hash_sha256_final(&state,
					 reinterpret_cast<unsigned char *>(out.data()));
#endif
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
#if defined(__APPLE__)
	CC_SHA256(reinterpret_cast<const unsigned char *>(src.data()),
		  src.size(), reinterpret_cast<unsigned char *>(out.data()));
#else
	crypto_hash_sha256(reinterpret_cast<unsigned char *>(out.data()),
			   reinterpret_cast<const unsigned char *>(src.data()),
			   src.size());
#endif
	return out;
}
