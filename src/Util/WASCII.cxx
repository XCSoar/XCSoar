/*
 * Copyright (C) 2011-2016 Max Kellermann <max.kellermann@gmail.com>
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
#include "CharUtil.hpp"

#include <assert.h>

void
CopyASCII(wchar_t *dest, const wchar_t *src)
{
	do {
		if (IsASCII(*src))
			*dest++ = *src;
	} while (*src++ != L'\0');
}

wchar_t *
CopyASCII(wchar_t *dest, size_t dest_size,
	  const wchar_t *src, const wchar_t *src_end)
{
	assert(dest != nullptr);
	assert(dest_size > 0);
	assert(src != nullptr);
	assert(src_end != nullptr);
	assert(src_end >= src);

	const wchar_t *const dest_end = dest + dest_size;
	for (; dest != dest_end && src != src_end; ++src)
		if (IsASCII(*src))
			*dest++ = *src;

	return dest;
}

void
CopyASCII(wchar_t *dest, const char *src)
{
	do {
		if (IsASCII(*src))
			*dest++ = (wchar_t)*src;
	} while (*src++ != '\0');
}

template<typename D, typename S>
static D *
TemplateCopyASCII(D *dest, size_t dest_size, const S *src, const S *src_end)
{
	assert(dest != nullptr);
	assert(dest_size > 0);
	assert(src != nullptr);
	assert(src_end != nullptr);
	assert(src_end >= src);

	const D *const dest_end = dest + dest_size;
	for (; dest != dest_end && src != src_end; ++src)
		if (IsASCII(*src))
			*dest++ = *src;

	return dest;
}

wchar_t *
CopyASCII(wchar_t *dest, size_t dest_size, const char *src, const char *src_end)
{
	return TemplateCopyASCII(dest, dest_size, src, src_end);
}

char *
CopyASCII(char *dest, size_t dest_size, const wchar_t *src, const wchar_t *src_end)
{
	return TemplateCopyASCII(dest, dest_size, src, src_end);
}

void
CopyASCIIUpper(char *dest, const wchar_t *src)
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
