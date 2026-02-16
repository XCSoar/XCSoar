// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The XCSoar Project

#pragma once

#include "util/Compiler.h"

#include <tchar.h>

/**
 * Replaces the "base name" of the specified path, i.e. the portion
 * after the last path separator.  If the input path does not contain
 * a directory name, the whole string is replaced.
 *
 * @param the input and output buffer
 * @param new_base the new base name to be copied to #path
 */
void
ReplaceBaseName(char *path, const char *new_base);
