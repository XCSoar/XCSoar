// SPDX-License-Identifier: BSD-2-Clause
// author: Max Kellermann <max.kellermann@gmail.com>

#pragma once

#include <cstddef>
#include <string_view>

#include <wchar.h>

[[gnu::nonnull]]
void
CopyASCII(wchar_t *dest, const wchar_t *src) noexcept;

[[gnu::nonnull]]
wchar_t *
CopyASCII(wchar_t *dest, std::size_t dest_size,
	  std::wstring_view src) noexcept;

[[gnu::nonnull]]
void
CopyASCII(wchar_t *dest, const char *src) noexcept;

[[gnu::nonnull]]
wchar_t *
CopyASCII(wchar_t *dest, std::size_t dest_size,
	  std::string_view src) noexcept;

[[gnu::nonnull]]
char *
CopyASCII(char *dest, std::size_t dest_size,
	  std::wstring_view src) noexcept;

[[gnu::nonnull]]
char *
CopyASCIIUpper(char *dest, std::size_t dest_size,
	       std::wstring_view src) noexcept;
