// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The XCSoar Project

#pragma once

#include <memory>

#include <tchar.h>

class Reader;

/**
 * Opens a file from the data directory.
 *
 * Throws exception on error.
 *
 * @param name the file name relative to the data directory
 */
std::unique_ptr<Reader>
OpenDataFile(const char *name);
