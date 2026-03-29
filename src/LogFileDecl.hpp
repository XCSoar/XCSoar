// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The XCSoar Project

#pragma once

#include "util/Compiler.h"

#include <exception>
#include <string_view>

/**
 * Write a line to the log file.
 *
 * @param s the line, which must not contain newline or carriage
 * return characters
 */
void
LogString(std::string_view s) noexcept;

/**
 * Write a formatted line to the log file.
 *
 * @param fmt the format string, which must not contain newline or
 * carriage return characters
 */
gcc_printf(1, 2)
void
LogFormat(const char *fmt, ...) noexcept;

void
LogError(std::exception_ptr e) noexcept;

void
LogError(std::exception_ptr e, const char *msg) noexcept;
