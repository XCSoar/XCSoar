// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The XCSoar Project

#pragma once

/**
 * Is this a "reserved" character?
 *
 * Note that this doesn't check for CR/LF because these aren't allowed
 * within a line anyway.  No idea why these are mentioned in the
 * specification.
 *
 * @see IGC specification, section A6
 */
static constexpr bool
IsReservedIGCChar(char ch) noexcept
{
  return ch == '$' || ch == '*' || ch == '!' || ch == '\\' ||
    ch == '^' || ch == '~';
}

/**
 * Is this a "valid" character?
 *
 * @see IGC specification, section A6
 */
static constexpr bool
IsValidIGCChar(char ch) noexcept
{
  return ch >= 0x20 && ch <= 0x7e && !IsReservedIGCChar(ch);
}

/**
 * Copy a null-terminated string to a buffer to be written to an IGC
 * file.  If the string is too long for the buffer, it is truncated.
 * The destination buffer will not be null-terminated.
 */
char *
CopyIGCString(char *dest, char *dest_limit, const char *src) noexcept;
