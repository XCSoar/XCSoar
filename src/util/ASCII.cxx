// SPDX-License-Identifier: BSD-2-Clause
// author: Max Kellermann <max.kellermann@gmail.com>

#include "ASCII.hxx"
#include "CharUtil.hxx"

#include <cassert>

void
CopyASCII(char *dest, const char *src) noexcept
{
	do {
		if (IsASCII(*src))
			*dest++ = *src;
	} while (*src++ != '\0');
}

char *
CopyASCII(char *dest, std::size_t dest_size,
	  std::string_view src) noexcept
{
	const auto dest_end = dest + dest_size;
	for (const auto ch : src) {
		if (!IsASCII(ch))
			continue;

		if (dest == dest_end)
			break;

		*dest++ = ch;
	}

	return dest;
}

void
CopyASCIIUpper(char *dest, const char *src) noexcept
{
	do {
		char ch = *src;
		if (IsASCII(ch)) {
			if (IsLowerAlphaASCII(ch))
				ch -= 'a' - 'A';

			*dest++ = ch;
		}
	} while (*src++ != '\0');
}
