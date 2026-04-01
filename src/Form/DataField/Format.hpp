// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The XCSoar Project

#pragma once

#include <cstdio>

/**
 * Small helper for formatting into fixed-size DataField buffers.
 *
 * This replaces ad-hoc sprintf() usage with a bounded snprintf()
 * without changing the format strings.
 */
template<std::size_t N, typename... Args>
static inline void
FormatDataField(char (&buffer)[N], const char *format, Args... args) noexcept
{
  std::snprintf(buffer, N, format, args...);
  buffer[N - 1] = 0;
}

