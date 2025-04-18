// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The XCSoar Project

#pragma once

#include "util/Compiler.h"

#include <cstddef>

/**
 * Copy a string, ignoring all characters that are illegal in a
 * setting value.  There is no buffer overflow check; the destination
 * buffer must be large enough to fit all of the source string
 * (worst-case).
 *
 * @param maxBytes copy no more than maxBytes bytes (excluding null terminator)
 */
char *
CopyCleanFlarmString(char *gcc_restrict dest, const char *gcc_restrict src,
                     std::size_t maxBytes) noexcept;
