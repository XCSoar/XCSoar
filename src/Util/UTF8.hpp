/*
Copyright_License {

  XCSoar Glide Computer - http://www.xcsoar.org/
  Copyright (C) 2000-2016 The XCSoar Project
  A detailed list of copyright holders can be found in the file "AUTHORS".

  This program is free software; you can redistribute it and/or
  modify it under the terms of the GNU General Public License
  as published by the Free Software Foundation; either version 2
  of the License, or (at your option) any later version.

  This program is distributed in the hope that it will be useful,
  but WITHOUT ANY WARRANTY; without even the implied warranty of
  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
  GNU General Public License for more details.

  You should have received a copy of the GNU General Public License
  along with this program; if not, write to the Free Software
  Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA  02111-1307, USA.
}
*/

#ifndef XCSOAR_UTIL_UTF8_HPP
#define XCSOAR_UTIL_UTF8_HPP

#include "Compiler.h"

#include <utility>

#include <stddef.h>

/**
 * Is this a valid UTF-8 string?
 */
gcc_pure  gcc_nonnull_all
bool
ValidateUTF8(const char *p);

/**
 * @return the number of the sequence beginning with the given
 * character, or 0 if the character is not a valid start byte
 */
gcc_const
size_t
SequenceLengthUTF8(char ch);

/**
 * @return the number of the first sequence in the given string, or 0
 * if the sequence is malformed
 */
gcc_pure
size_t
SequenceLengthUTF8(const char *p);

/**
 * Convert the specified string from ISO-8859-1 to UTF-8.
 *
 * @return the UTF-8 version of the source string; may return #src if
 * there are no non-ASCII characters; returns nullptr if the destination
 * buffer is too small
 */
gcc_pure  gcc_nonnull_all
const char *
Latin1ToUTF8(const char *src, char *buffer, size_t buffer_size);

/**
 * Convert the specified character from ISO-8859-1 to UTF-8 and write
 * it to the buffer.  buffer must have a length of at least 2!
 *
 * @return a pointer to the buffer plus the added bytes(s)
 */
gcc_nonnull_all
char *
Latin1ToUTF8(unsigned char ch, char *buffer);

/**
 * Convert the specified Unicode character to UTF-8 and write it to
 * the buffer.  buffer must have a length of at least 6!
 *
 * @return a pointer to the buffer plus the added bytes(s)
 */
gcc_nonnull_all
char *
UnicodeToUTF8(unsigned ch, char *buffer);

/**
 * Returns the number of characters in the string.  This is different
 * from strlen(), which counts the number of bytes.
 */
gcc_pure gcc_nonnull_all
size_t
LengthUTF8(const char *p);

/**
 * Check if the string ends with an incomplete UTF-8 multi-byte
 * sequence, and if so, insert a null terminator at the first byte of
 * the incomplete sequence.
 *
 * Call this after truncating an UTF-8 string to eliminate the last
 * incomplete sequence.
 *
 * The rest of the string must be valid UTF-8.
 *
 * @return a pointer to the new null terminator
 */
gcc_nonnull_all
char *
CropIncompleteUTF8(char *p);

/**
 * Return the number of bytes representing the first #max_chars
 * characters of a null-terminated string.  If the string has fewer
 * characters, the string length (in bytes) is returned.  No partial
 * multi-byte sequence will be considered.
 */
gcc_pure gcc_nonnull_all
size_t
TruncateStringUTF8(const char *p, size_t max_chars, size_t max_bytes);

/**
 * Copy a string to a buffer, truncating it if the buffer is not large
 * enough.  At most #truncate characters will be copied.  No partial
 * multi-byte sequence will be copied.
 *
 * @param dest_size the total size of the destination buffer, which
 * includes the null byte
 * @param truncate the maximum number of characters (not bytes) to
 * copy
 * @return a pointer to the end of the destination string
 */
char *
CopyTruncateStringUTF8(char *dest, size_t dest_size,
                       const char *src, size_t truncate);

/**
 * Decode the next UNICODE character.
 *
 * @param p a null-terminated valid UTF-8 string
 * @return a pair containing the next UNICODE character code and a
 * pointer to the first byte of the following character or 0 if
 * already at the end of the string
 */
gcc_pure gcc_nonnull_all
std::pair<unsigned, const char *>
NextUTF8(const char *p);

#endif
