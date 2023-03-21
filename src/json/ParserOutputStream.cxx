// SPDX-License-Identifier: BSD-2-Clause
// Copyright Max Kellermann <max.kellermann@gmail.com>

#include "ParserOutputStream.hxx"

namespace Json {

void
ParserOutputStream::Write(const void *data, size_t size)
{
	parser.write((const char *)data, size);
}

} // namespace Json
