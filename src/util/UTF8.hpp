// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The XCSoar Project

#pragma once

#include <cstddef>
#include <span>
#include <string_view>
#include <utility>

constexpr std::string_view utf8_byte_order_mark{"\xef\xbb\xbf"};

/**
 * Is this a valid UTF-8 string?
 */
[[gnu::pure]]  [[gnu::nonnull]]
bool
ValidateUTF8(const char *p) noexcept;

/**
 * Is this a valid UTF-8 string?
 */
[[gnu::pure]]
bool
ValidateUTF8(std::string_view p) noexcept;

/**
 * @return the number of the sequence beginning with the given
 * character, or 0 if the character is not a valid start byte
 */
[[gnu::const]]
std::size_t
SequenceLengthUTF8(char ch) noexcept;

/**
 * @return the number of the first sequence in the given string, or 0
 * if the sequence is malformed
 */
[[gnu::pure]]
std::size_t
SequenceLengthUTF8(const char *p) noexcept;

/**
 * Convert the specified string from ISO-8859-1 to UTF-8.
 *
 * @return the UTF-8 version of the source string; may return #src if
 * there are no non-ASCII characters; returns nullptr if the destination
 * buffer is too small
 */
[[gnu::pure]] [[gnu::nonnull]]
const char *
Latin1ToUTF8(const char *src, std::span<char> buffer) noexcept;

[[gnu::pure]]
std::string_view
Latin1ToUTF8(std::string_view src, std::span<char> buffer) noexcept;

/**
 * Convert the specified character from ISO-8859-1 to UTF-8 and write
 * it to the buffer.  buffer must have a length of at least 2!
 *
 * @return a pointer to the buffer plus the added bytes(s)
 */
[[gnu::nonnull]]
char *
Latin1ToUTF8(unsigned char ch, char *buffer) noexcept;

/**
 * Convert the specified Unicode character to UTF-8 and write it to
 * the buffer.  buffer must have a length of at least 6!
 *
 * @return a pointer to the buffer plus the added bytes(s)
 */
[[gnu::nonnull]]
char *
UnicodeToUTF8(unsigned ch, char *buffer) noexcept;

/**
 * Returns the number of characters in the string.  This is different
 * from strlen(), which counts the number of bytes.
 */
[[gnu::pure]] [[gnu::nonnull]]
std::size_t
LengthUTF8(const char *p) noexcept;

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
[[gnu::nonnull]]
char *
CropIncompleteUTF8(char *p) noexcept;

/**
 * Return the number of bytes representing the first #max_chars
 * characters of a string.  If the string has fewer
 * characters, the string length (in bytes) is returned.  No partial
 * multi-byte sequence will be considered.
 */
[[gnu::pure]]
std::size_t
TruncateStringUTF8(std::string_view s, std::size_t max_chars) noexcept;

/**
 * Return the number of bytes representing the first #max_chars
 * characters of a null-terminated string.  If the string has fewer
 * characters, the string length (in bytes) is returned.  No partial
 * multi-byte sequence will be considered.
 */
[[gnu::pure]] [[gnu::nonnull]]
std::size_t
TruncateStringUTF8(const char *p,
                   std::size_t max_chars, std::size_t max_bytes) noexcept;

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
CopyTruncateStringUTF8(std::span<char> dest,
                       const char *src, std::size_t truncate) noexcept;

/**
 * Decode the next UNICODE character.
 *
 * @param p a null-terminated valid UTF-8 string
 * @return a pair containing the next UNICODE character code and a
 * pointer to the first byte of the following character or 0 if
 * already at the end of the string
 */
[[gnu::pure]] [[gnu::nonnull]]
std::pair<unsigned, const char *>
NextUTF8(const char *p) noexcept;
