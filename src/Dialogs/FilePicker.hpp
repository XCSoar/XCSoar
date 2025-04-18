// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The XCSoar Project

#pragma once

#include <tchar.h>

class AllocatedPath;
class FileDataField;

bool
FilePicker(const TCHAR *caption, FileDataField &df,
           const TCHAR *help_text = nullptr);

/**
 * Ask the user to pick a file from the data directory.
 *
 * @param patterns a list of shell patterns (such as "*.xcm")
 * separated by null bytes and terminated by an empty string
 * @return an absolute file name, or nullptr false if the user has
 * cancelled the dialog or if there are no matching files
 */
AllocatedPath
FilePicker(const TCHAR *caption, const TCHAR *patterns);
