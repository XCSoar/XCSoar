// SPDX-License-Identifier: BSD-2-Clause
// author: Max Kellermann <max.kellermann@gmail.com>

#pragma once

#include <cstddef>
#include <string_view>

/**
 * Copy all ASCII characters to the destination string
 * (i.e. 0x01..0x7f), ignoring the others.  In the worst case, the
 * destination buffer must be as large as the source buffer.  Can be
 * used for in-place operation.
 */
[[gnu::nonnull]]
void
CopyASCII(char *dest, const char *src) noexcept;

/**
 * Copy all ASCII characters to the destination string
 * (i.e. 0x01..0x7f), ignoring the others.
 *
 * This function does not null-terminate the destination buffer.
 *
 * @param dest_size the size of the destination buffer
 * @return a pointer to the written end of the destination buffer
 */
[[gnu::nonnull]]
char *
CopyASCII(char *dest, std::size_t dest_size,
	  std::string_view src) noexcept;

/**
 * Like CopyASCII(), but convert all letters to upper-case.
 */
[[gnu::nonnull]]
char *
CopyASCIIUpper(char *dest, std::size_t dest_size,
	       std::string_view src) noexcept;
