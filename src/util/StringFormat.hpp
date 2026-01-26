// SPDX-License-Identifier: BSD-2-Clause
// author: Max Kellermann <max.kellermann@gmail.com>

#pragma once

#include <stdio.h>

template<typename... Args>
static inline int
StringFormat(char *buffer, size_t size, const char *fmt,
	     Args&&... args) noexcept
{
  return snprintf(buffer, size, fmt, args...);
}

template<typename... Args>
static inline int
StringFormatUnsafe(char *buffer, const char *fmt, Args&&... args) noexcept
{
  return sprintf(buffer, fmt, args...);
}
