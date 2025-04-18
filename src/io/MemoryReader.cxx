// SPDX-License-Identifier: BSD-2-Clause
// author: Max Kellermann <max.kellermann@gmail.com>

#include "MemoryReader.hxx"

#include <algorithm>

std::size_t
MemoryReader::Read(std::span<std::byte> dest)
{
	auto src = buffer;
	if (src.size() > dest.size())
		src = src.first(dest.size());
	buffer = buffer.subspan(src.size());

	std::copy(src.begin(), src.end(), dest.begin());
	return src.size();
}
