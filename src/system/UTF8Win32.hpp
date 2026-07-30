// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The XCSoar Project

#pragma once

#ifdef _WIN32

#include <string>
#include <string_view>

/** UTF-8 → UTF-16 for Win32 *W APIs. Empty on failure. */
[[nodiscard]] [[gnu::pure]]
std::wstring
UTF8ToWide(std::string_view s) noexcept;

/** UTF-16 → UTF-8 from Win32 *W APIs. Empty on failure. */
[[nodiscard]] [[gnu::pure]]
std::string
WideToUTF8(std::wstring_view s) noexcept;

[[nodiscard]] [[gnu::pure]]
inline std::string
WideToUTF8(const wchar_t *s) noexcept
{
  if (s == nullptr || *s == L'\0')
    return {};

  return WideToUTF8(std::wstring_view(s));
}

#endif
