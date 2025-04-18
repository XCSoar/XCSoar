// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The XCSoar Project
#pragma once

#include <memory>
#include <string_view>

class FileReader;

/**
 * Opens a file whose name is configured in the profile.
 *
 * Throws on error.
 *
 * @param profile_key the profile key which is used to read the
 * file name
 * @return a FileReader; nullptr if there is no such setting
 */
std::unique_ptr<FileReader>
OpenConfiguredFile(std::string_view profile_key);
