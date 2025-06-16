// SPDX-License-Identifier: BSD-2-Clause
// author: Max Kellermann <max.kellermann@gmail.com>

#pragma once

#include <stdio.h>

#ifdef _WIN32
#include <string.h>
#endif

template <typename... Args>
static inline int
StringFormat(wchar_t *buffer, size_t size, const wchar_t *fmt,
             Args &&...args) noexcept
{
  return snwprintf(buffer, size, fmt, args...);
}

template <typename... Args>
static inline int
StringFormatUnsafe(wchar_t *buffer, const wchar_t *fmt,
                   Args &&...args) noexcept
{
  return swprintf(buffer, fmt, args...);
}
