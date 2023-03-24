// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The XCSoar Project

#pragma once

#include "util/Compiler.h"

#include <exception>

#ifdef _UNICODE
#include <wchar.h>
#endif

/**
 * Write a formatted line to the log file.
 *
 * @param fmt the format string, which must not contain newline or
 * carriage return characters
 */
gcc_printf(1, 2)
void
LogFormat(const char *fmt, ...) noexcept;

#ifdef _UNICODE
void
LogFormat(const wchar_t *fmt, ...) noexcept;
#endif

#if !defined(NDEBUG)

#define LogDebug(...) LogFormat(__VA_ARGS__)

#else /* NDEBUG */

/* not using an empty inline function here because we don't want to
   evaluate the parameters */
#define LogDebug(...) do {} while (false)

#endif /* NDEBUG */

void
LogError(std::exception_ptr e) noexcept;

void
LogError(std::exception_ptr e, const char *msg) noexcept;
