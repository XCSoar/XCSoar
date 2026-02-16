// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The XCSoar Project

#pragma once

#include <cstddef>

/**
 * Copy a string to a buffer, truncating it if the buffer is not large
 * enough.  No partial (UTF-8) multi-byte sequence will be copied.
 *
 * @param dest_size the total size of the destination buffer, which
 * includes the null byte
 * @return a pointer to the end of the destination string
 */
char *
CopyTruncateString(char *dest, size_t dest_size, const char *src);

/**
 * Copy a string to a buffer, truncating it if the buffer is not large
 * enough.  At most #truncate characters will be copied.  No partial
 * (UTF-8) multi-byte sequence will be copied.
 *
 * @param dest_size the total size of the destination buffer, which
 * includes the null byte
 * @param truncate the maximum number of characters (not bytes) to
 * copy
 * @return a pointer to the end of the destination string
 */
char *
CopyTruncateString(char *dest, size_t dest_size,
                   const char *src, size_t truncate);
