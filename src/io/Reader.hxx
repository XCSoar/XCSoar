// SPDX-License-Identifier: BSD-2-Clause
// author: Max Kellermann <max.kellermann@gmail.com>

#pragma once

#include <cstddef>
#include <cstdint>
#include <span>
#include <type_traits>

/**
 * An interface that can read bytes from a stream until the stream
 * ends.
 *
 * This interface is simpler and less cumbersome to use than
 * #InputStream.
 */
class Reader {
public:
	Reader() = default;
	Reader(const Reader &) = delete;

	virtual ~Reader() noexcept = default;

	/**
	 * Return the total size of the stream in bytes, or 0 if
	 * unknown.  Used for progress reporting.
	 */
	[[nodiscard]] [[gnu::pure]]
	virtual uint_least64_t GetSize() const noexcept {
		return 0;
	}

	/**
	 * Read data from the stream.
	 *
	 * @return the number of bytes read into the given buffer or 0
	 * on end-of-stream
	 */
	[[nodiscard]]
	virtual std::size_t Read(std::span<std::byte> dest) = 0;

	/**
	 * Like Read(), but throws an exception when there is not
	 * enough data to fill the destination buffer.
	 */
	void ReadFull(std::span<std::byte> dest);

	template<typename T>
	requires std::is_standard_layout_v<T> && std::is_trivially_copyable_v<T>
	void ReadT(T &dest) {
		ReadFull(std::as_writable_bytes(std::span{&dest, 1}));
	}
};
