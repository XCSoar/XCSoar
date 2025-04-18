// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The XCSoar Project

#pragma once

#include <cstddef>

/**
 * Copy a string to a fixed-size buffer.  The destination buffer is
 * not null-terminated, it is padded with spaces at the end if the
 * source string is shorter than the buffer.  If the source string is
 * longer than the destination buffer, it is clipped.
 */
void
copy_padded(char *dest, size_t size, const char *src);

/**
 * Copy a string from a fixed-size buffer that may not be
 * null-terminated, converting ASCII lower-case letters to upper case.
 * The destination buffer will be null-terminated.
 *
 * @param size the size of the source buffer
 */
void
CopyTerminatedUpper(char *dest, const char *src, size_t size);

/**
 * Copy a string to a fixed-size buffer, converting ASCII lower-case
 * letters to upper case.  The destination buffer is not
 * null-terminated, it is padded with spaces at the end if the source
 * string is shorter than the buffer.  If the source string is longer
 * than the destination buffer, it is clipped.
 */
void
CopyPaddedUpper(char *dest, size_t size, const char *src);
