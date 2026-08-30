// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The XCSoar Project

#pragma once

#include "Repository/AvailableFile.hpp"
#include "Repository/FileType.hpp"
#include "system/Path.hpp"

#include <vector>

/**
 * Download repository files of the given type.
 *
 * @param allow_multi_select when false, the picker chooses one file
 * (used by FilePicker, which configures a single path).
 */
std::vector<AllocatedPath>
DownloadFilePicker(FileType file_type, bool allow_multi_select = true);

/**
 * Pick one or more repository files.  Groups by country/area when
 * more than one area is present.  Returns the chosen files (not
 * downloaded).  Empty if the user cancelled.
 */
std::vector<AvailableFile>
SelectAvailableFiles(std::vector<AvailableFile> files);
