// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The XCSoar Project

#pragma once

#include <string_view>

#include <wchar.h>

/**
 * Copy a string.  If the buffer is too small, then the string is
 * truncated.  This is a safer version of strncpy().
 *
 * @param dest_size the size of the destination buffer (including the
 * null terminator)
 * @return a pointer to the null terminator
 */
[[gnu::nonnull]]
wchar_t *
CopyString(wchar_t *dest, size_t dest_size, std::wstring_view src) noexcept;

[[gnu::nonnull]]
wchar_t *
NormalizeSearchString(wchar_t *dest, std::wstring_view src) noexcept;
