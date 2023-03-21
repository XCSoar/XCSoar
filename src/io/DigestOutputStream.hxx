// SPDX-License-Identifier: BSD-2-Clause
// author: Max Kellermann <max.kellermann@gmail.com>

#pragma once

#include "io/OutputStream.hxx"

#include <span>

/**
 * An #OutputStream wrapper which calculates a digest.
 */
template<typename T>
class DigestOutputStream final : public OutputStream {
	OutputStream &next;

	T state;

public:
	explicit DigestOutputStream(OutputStream &_next) noexcept
		:next(_next) {}

	void Final(void *dest) noexcept {
		state.Final(dest);
	}

	auto Final() noexcept {
		return state.Final();
	}

	/* virtual methods from class OutputStream */
	void Write(const void *data, size_t size) override {
		next.Write(data, size);
		state.Update(std::span<const std::byte>{(const std::byte *)data, size});
	}
};
