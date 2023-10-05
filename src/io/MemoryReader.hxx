// SPDX-License-Identifier: BSD-2-Clause
// author: Max Kellermann <max.kellermann@gmail.com>

#pragma once

#include "Reader.hxx"

/**
 * A #Reader implementation which reads from a memory buffer.
 */
class MemoryReader : public Reader {
	std::span<const std::byte> buffer;

public:
	explicit MemoryReader(std::span<const std::byte> _buffer) noexcept
		:buffer(_buffer) {}

	/* virtual methods from class Reader */
	std::size_t Read(std::span<std::byte> dest) override;
};
