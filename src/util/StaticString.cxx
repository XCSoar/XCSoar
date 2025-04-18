// SPDX-License-Identifier: BSD-2-Clause
// author: Max Kellermann <max.kellermann@gmail.com>

#include "StaticString.hxx"

#ifdef _UNICODE
#include <stringapiset.h>
#endif

bool
CopyUTF8(char *dest, size_t dest_size, const char *src) noexcept
{
	if (!::ValidateUTF8(src))
		return false;

	CopyString(dest, dest_size, src);
	CropIncompleteUTF8(dest);
	return true;
}

#ifdef _UNICODE

bool
CopyUTF8(wchar_t *dest, size_t dest_size, const char *src) noexcept
{
	int result = MultiByteToWideChar(CP_UTF8, 0, src, -1, dest, dest_size);
	return result > 0;
}

#endif
