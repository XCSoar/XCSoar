// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The XCSoar Project

#pragma once

#include <cstddef>

class AllocatedString;

/**
 * Uncompress the given buffer, and return it as a C string.  Returns
 * nullptr on error.
 */
AllocatedString
InflateToString(const void *compressed, size_t length) noexcept;
