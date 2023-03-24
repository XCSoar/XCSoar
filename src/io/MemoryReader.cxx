// SPDX-License-Identifier: BSD-2-Clause
// author: Max Kellermann <max.kellermann@gmail.com>

#include "MemoryReader.hxx"

#include <string.h>

std::size_t
MemoryReader::Read(void *data, std::size_t size)
{
	if (size > buffer.size())
		size = buffer.size();

	memcpy(data, buffer.data(), size);
	buffer = buffer.subspan(size);
	return size;
}
