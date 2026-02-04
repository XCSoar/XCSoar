// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The XCSoar Project

#pragma once

#include <string>

/**
 * Unescape common C-style and octal/hex escapes in a string.
 * Supports: \r, \n, \\ , \xHH, \OOO (up to 3 octal digits).
 * Returns a new std::string with escapes decoded. Never returns null.
 */
std::string UnescapeCString(const std::string &s) noexcept;
