// SPDX-License-Identifier: BSD-2-Clause
// Copyright Max Kellermann <max.kellermann@gmail.com>

#include "ParserOutputStream.hxx"
#include "util/SpanCast.hxx"

namespace Json {

void
ParserOutputStream::Write(std::span<const std::byte> src)
{
	parser.write(ToStringView(src));
}

} // namespace Json
