// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The XCSoar Project

#pragma once

#ifdef _WIN32

#include <string>
#include <string_view>

/**
 * Convert between UTF-8 #Path strings and UTF-16 for Win32 *W APIs.
 *
 * Never pass ANSI/ACP bytes (from CreateFileA, GetModuleFileNameA,
 * fopen, …) into UTF8ToWide — that asserts and fails (#2824). Obtain
 * paths with *W APIs + WideToUTF8, or open files with OpenPathFile() /
 * CreateFileW(UTF8ToWide(...)).
 */

/** UTF-8 → UTF-16 for Win32 *W APIs. Empty on failure. */
[[nodiscard]] [[gnu::pure]]
std::wstring
UTF8ToWide(std::string_view s) noexcept;

/** Like UTF8ToWide(string_view); empty if @p s is null or empty. */
[[nodiscard]] [[gnu::pure]]
inline std::wstring
UTF8ToWide(const char *s) noexcept
{
  if (s == nullptr || *s == '\0')
    return {};

  return UTF8ToWide(std::string_view(s));
}

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
