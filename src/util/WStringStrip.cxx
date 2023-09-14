// SPDX-License-Identifier: BSD-2-Clause
// author: Max Kellermann <max.kellermann@gmail.com>

#include "WStringStrip.hxx"
#include "WStringAPI.hxx"
#include "WCharUtil.hxx"

#include <algorithm>
#include <cstring>

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

std::wstring_view
StripLeft(const std::wstring_view s) noexcept
{
	auto i = std::find_if_not(s.begin(), s.end(),
				  [](auto ch){ return IsWhitespaceOrNull(ch); });

	return {
		i,
		s.end(),
	};
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

std::wstring_view
StripRight(std::wstring_view s) noexcept
{
	auto i = std::find_if_not(s.rbegin(), s.rend(),
				  [](auto ch){ return IsWhitespaceOrNull(ch); });

	return s.substr(0, std::distance(i, s.rend()));
}

wchar_t *
Strip(wchar_t *p) noexcept
{
	p = StripLeft(p);
	StripRight(p);
	return p;
}

std::wstring_view
Strip(std::wstring_view s) noexcept
{
	return StripRight(StripLeft(s));
}
