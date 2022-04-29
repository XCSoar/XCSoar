/*
 * Copyright 2020 Max Kellermann <max.kellermann@gmail.com>
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

#include "util/ConstBuffer.hxx"

#include <sodium/crypto_hash_sha256.h>

#include <array>
#include <cstddef> // for std::byte

using SHA256Digest = std::array<std::byte, crypto_hash_sha256_BYTES>;

class SHA256State {
	crypto_hash_sha256_state state;

public:
	SHA256State() noexcept {
		crypto_hash_sha256_init(&state);
	}

	void Update(ConstBuffer<void> p) noexcept {
		crypto_hash_sha256_update(&state,
					  (const unsigned char *)p.data,
					  p.size);
	}

	template<typename T>
	void UpdateT(const T &p) noexcept {
		Update({&p, sizeof(p)});
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
