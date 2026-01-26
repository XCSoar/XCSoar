// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The XCSoar Project

#pragma once

#include <cstddef>
#include <string_view>

/**
 * Copy a string.  If the buffer is too small, then the string is
 * truncated.  This is a safer version of strncpy().
 *
 * @param dest_size the size of the destination buffer (including the
 * null terminator)
 * @return a pointer to the null terminator
 */
[[gnu::nonnull]]
char *
CopyString(char *dest, size_t dest_size, std::string_view src) noexcept;

/**
 * Normalize a string for searching.  This strips all characters
 * except letters and digits, folds case to a neutral form.  It is
 * possible to do this in-place (src==dest).
 *
 * @param dest the destination buffer; there must be enough room for
 * the source string and the trailing zero
 * @param src the source string
 * @return the destination buffer
 */
[[gnu::nonnull]]
char *
NormalizeSearchString(char *dest, std::string_view src) noexcept;
