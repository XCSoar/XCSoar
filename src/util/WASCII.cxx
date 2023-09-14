// SPDX-License-Identifier: BSD-2-Clause
// author: Max Kellermann <max.kellermann@gmail.com>

#include "WASCII.hxx"
#include "WCharUtil.hxx"

#include <cassert>

template<typename D, typename S>
static D *
TemplateCopyASCII(D *dest, std::size_t dest_size,
		  std::basic_string_view<S> src) noexcept
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
CopyASCII(wchar_t *dest, const wchar_t *src) noexcept
{
	do {
		if (IsASCII(*src))
			*dest++ = *src;
	} while (*src++ != L'\0');
}

wchar_t *
CopyASCII(wchar_t *dest, std::size_t dest_size,
	  std::wstring_view src) noexcept
{
	return TemplateCopyASCII(dest, dest_size, src);
}

void
CopyASCII(wchar_t *dest, const char *src) noexcept
{
	do {
		if (IsASCII(*src))
			*dest++ = (wchar_t)*src;
	} while (*src++ != '\0');
}

wchar_t *
CopyASCII(wchar_t *dest, std::size_t dest_size,
	  std::string_view src) noexcept
{
	return TemplateCopyASCII(dest, dest_size, src);
}

char *
CopyASCII(char *dest, std::size_t dest_size,
	  std::wstring_view src) noexcept
{
	return TemplateCopyASCII(dest, dest_size, src);
}

char *
CopyASCIIUpper(char *dest, std::size_t dest_size,
	       std::wstring_view src) noexcept
{
	const auto dest_end = dest + dest_size;
	for (auto t : src) {
		if (!IsASCII(t))
			continue;

		if (dest == dest_end)
			break;

		char ch = (char)t;
		*dest++ = ToUpperASCII(ch);
	}

	return dest;
}
