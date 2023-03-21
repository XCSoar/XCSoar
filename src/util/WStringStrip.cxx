// SPDX-License-Identifier: BSD-2-Clause
// author: Max Kellermann <max.kellermann@gmail.com>

#include "WStringStrip.hxx"
#include "WStringAPI.hxx"
#include "WCharUtil.hxx"

#include <string.h>

const wchar_t *
StripLeft(const wchar_t *p) noexcept
{
	while (IsWhitespaceNotNull(*p))
		++p;

	return p;
}

const wchar_t *
StripLeft(const wchar_t *p, const wchar_t *end) noexcept
{
	while (p < end && IsWhitespaceOrNull(*p))
		++p;

	return p;
}

const wchar_t *
StripRight(const wchar_t *p, const wchar_t *end) noexcept
{
	while (end > p && IsWhitespaceOrNull(end[-1]))
		--end;

	return end;
}

size_t
StripRight(const wchar_t *p, size_t length) noexcept
{
	while (length > 0 && IsWhitespaceOrNull(p[length - 1]))
		--length;

	return length;
}

void
StripRight(wchar_t *p) noexcept
{
	size_t old_length = StringLength(p);
	size_t new_length = StripRight(p, old_length);
	p[new_length] = 0;
}

wchar_t *
Strip(wchar_t *p) noexcept
{
	p = StripLeft(p);
	StripRight(p);
	return p;
}
