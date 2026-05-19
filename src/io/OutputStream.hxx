// SPDX-License-Identifier: BSD-2-Clause
// author: Max Kellermann <max.kellermann@gmail.com>

#pragma once

#include <cstddef>
#include <span>

class OutputStream {
public:
	OutputStream() = default;
	OutputStream(const OutputStream &) = delete;
	virtual ~OutputStream() noexcept = default;

	/**
	 * Throws std::exception on error.
	 */
	virtual void Write(std::span<const std::byte> src) = 0;

	/**
	 * Finalize the stream: flush all data and make it
	 * permanent.  After returning, this object must not be
	 * used for writing again.
	 *
	 * The default implementation does nothing, which is
	 * suitable for in-memory or stdio streams.  Subclasses
	 * that need explicit finalization (e.g. atomic-rename,
	 * SAF flush+close) must override.
	 *
	 * Throws on error.
	 */
	virtual void Commit() {}
};
