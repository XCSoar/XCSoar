// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The XCSoar Project

#pragma once

#include "Charset.hpp"

#include <memory>
#include <tchar.h>

class Reader;
class TLineReader;
class NLineReader;

/**
 * Opens a file from the data directory.
 *
 * Throws exception on error.
 *
 * @param name the file name relative to the data directory
 */
std::unique_ptr<Reader>
OpenDataFile(const TCHAR *name);

/**
 * Opens a text file from the data directory.
 *
 * Throws exception on error.
 *
 * @param name the file name relative to the data directory
 * @param cs the character set of the input file
 */
std::unique_ptr<TLineReader>
OpenDataTextFile(const TCHAR *name, Charset cs=Charset::UTF8);

/**
 * Opens a text file from the data directory.
 *
 * Throws exception on error.
 *
 * @param name the file name relative to the data directory
 */
std::unique_ptr<NLineReader>
OpenDataTextFileA(const TCHAR *name);
