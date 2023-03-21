// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The XCSoar Project
#pragma once

#include "Charset.hpp"

#include <memory>

class NLineReader;
class TLineReader;

/**
 * Opens a file whose name is configured in the profile.
 *
 * @param profile_key the profile key which is used to read the
 * file name
 * @return a NLineReader; nullptr
 * if there is no such setting, or if an error occurred opening the
 * file
 */
std::unique_ptr<NLineReader>
OpenConfiguredTextFileA(const char *profile_key);

/**
 * Opens a file whose name is configured in the profile.
 *
 * @param profile_key the profile key which is used to read the
 * file name
 * @param cs the character set of the input file
 * @return a TLineReader; nullptr if
 * there is no such setting, or if an error occurred opening the file
 */
std::unique_ptr<TLineReader>
OpenConfiguredTextFile(const char *profile_key,
                       Charset cs=Charset::UTF8);

/**
 * Opens a file whose name is configured in the profile.  If there is
 * no such setting, attempt to open a file from inside the map file.
 *
 * @param profile_key the profile key which is used to read the
 * file name
 * @param in_map_file if no profile setting is found, attempt to open
 * this file from inside the map file
 * @param cs the character set of the input file
 * @return a TLineReader; nullptr if
 * there is no such setting, or if an error occurred opening the file
 */
std::unique_ptr<TLineReader>
OpenConfiguredTextFile(const char *profile_key, const char *in_map_file,
                       Charset cs=Charset::UTF8);
