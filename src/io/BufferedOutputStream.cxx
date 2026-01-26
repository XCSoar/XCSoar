// SPDX-License-Identifier: BSD-2-Clause
// author: Max Kellermann <max.kellermann@gmail.com>

#include "BufferedOutputStream.hxx"
#include "OutputStream.hxx"
#include "util/SpanCast.hxx"

#include <fmt/format.h>

#include <cstdarg>

#include <string.h>

bool
BufferedOutputStream::AppendToBuffer(std::span<const std::byte> src) noexcept
{
	auto w = buffer.Write();
	if (w.size() < src.size())
		return false;

	std::copy(src.begin(), src.end(), w.begin());
	buffer.Append(src.size());
	return true;
}

void
BufferedOutputStream::Write(std::span<const std::byte> src)
{
	/* try to append to the current buffer */
	if (AppendToBuffer(src))
		return;

	/* not enough room in the buffer - flush it */
	Flush();

	/* see if there's now enough room */
	if (AppendToBuffer(src))
		return;

	/* too large for the buffer: direct write */
	os.Write(src);
}

void
BufferedOutputStream::VFmt(fmt::string_view format_str, fmt::format_args args)
{
	/* TODO format into this object's buffer instead of allocating
	   a new one */
	fmt::memory_buffer b;
#if FMT_VERSION >= 80000
	fmt::vformat_to(std::back_inserter(b), format_str, args);
#else
	fmt::vformat_to(b, format_str, args);
#endif
	return Write(std::as_bytes(std::span{b.data(), b.size()}));
}

void
BufferedOutputStream::Flush()
{
	auto r = buffer.Read();
	if (r.empty())
		return;

	os.Write(r);
	buffer.Consume(r.size());
}
