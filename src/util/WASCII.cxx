/*
 * Copyright 2011-2022 Max Kellermann <max.kellermann@gmail.com>
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 *
 * - Redistributions of source code must retain the above copyright
 * notice, this list of conditions and the following disclaimer.
 *
 * - Redistributions in binary form must reproduce the above copyright
 * notice, this list of conditions and the following disclaimer in the
 * documentation and/or other materials provided with the
 * distribution.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
 * ``AS IS'' AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
 * LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS
 * FOR A PARTICULAR PURPOSE ARE DISCLAIMED.  IN NO EVENT SHALL THE
 * FOUNDATION OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT,
 * INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES
 * (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR
 * SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT,
 * STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
 * ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED
 * OF THE POSSIBILITY OF SUCH DAMAGE.
 */

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

void
CopyASCIIUpper(char *dest, const wchar_t *src) noexcept
{
	do {
		wchar_t t = *src;
		if (IsASCII(t)) {
			char ch = (char)t;
			if (IsLowerAlphaASCII(ch))
				ch -= 'a' - 'A';

			*dest++ = ch;
		}
	} while (*src++ != '\0');
}
