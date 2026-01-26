// SPDX-License-Identifier: BSD-2-Clause
// author: Max Kellermann <max.kellermann@gmail.com>

#include "StaticString.hxx"

bool
CopyUTF8(char *dest, size_t dest_size, const char *src) noexcept
{
	if (!::ValidateUTF8(src))
		return false;

	CopyString(dest, dest_size, src);
	CropIncompleteUTF8(dest);
	return true;
}
