// SPDX-License-Identifier: BSD-2-Clause
// author: Max Kellermann <max.kellermann@gmail.com>

#pragma once

#include "util/Compiler.h"
#include "util/DynamicFifoBuffer.hxx"

#include <cstddef>
#include <span>

#ifdef _UNICODE
#include <wchar.h>
#endif

class OutputStream;

/**
 * An #OutputStream wrapper that buffers its output to reduce the
 * number of OutputStream::Write() calls.
 *
 * All wchar_t based strings are converted to UTF-8.
 *
 * To make sure everything is written to the underlying #OutputStream,
 * call Flush() before destructing this object.
 */
class BufferedOutputStream {
	OutputStream &os;

	DynamicFifoBuffer<std::byte> buffer;

public:
	explicit BufferedOutputStream(OutputStream &_os,
				      size_t buffer_size=32768) noexcept
		:os(_os), buffer(buffer_size) {}

	/**
	 * Write the contents of a buffer.
	 */
	void Write(const void *data, std::size_t size);

	void Write(std::span<const std::byte> src) {
		Write(src.data(), src.size());
	}

	/**
	 * Write the given object.  Note that this is only safe with
	 * POD types.  Types with padding can expose sensitive data.
	 */
	template<typename T>
	void WriteT(const T &value) {
		Write(&value, sizeof(value));
	}

	/**
	 * Write one narrow character.
	 */
	void Write(const char &ch) {
		WriteT(ch);
	}

	/**
	 * Write a null-terminated string.
	 */
	void Write(const char *p);

	/**
	 * Write a printf-style formatted string.
	 */
	gcc_printf(2,3)
	void Format(const char *fmt, ...);

#ifdef _UNICODE
	/**
	 * Write one narrow character.
	 */
	void Write(const wchar_t &ch) {
		WriteWideToUTF8(&ch, 1);
	}

	/**
	 * Write a null-terminated wide string.
	 */
	void Write(const wchar_t *p);
#endif

	/**
	 * Finish the current line.
	 */
	void NewLine() {
#ifdef _WIN32
		Write('\r');
#endif
		Write('\n');
	}

	/**
	 * Write buffer contents to the #OutputStream.
	 */
	void Flush();

	/**
	 * Discard buffer contents.
	 */
	void Discard() noexcept {
		buffer.Clear();
	}

private:
	bool AppendToBuffer(const void *data, std::size_t size) noexcept;

#ifdef _UNICODE
	void WriteWideToUTF8(const wchar_t *p, std::size_t length);
#endif
};

/**
 * Helper function which constructs a #BufferedOutputStream, calls the
 * given function and flushes the #BufferedOutputStream.
 */
template<typename F>
void
WithBufferedOutputStream(OutputStream &os, F &&f)
{
	BufferedOutputStream bos(os);
	f(bos);
	bos.Flush();
}
