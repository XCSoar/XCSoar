// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The XCSoar Project

#pragma once

#include "Repository/AvailableFile.hpp"
#include "Repository/FileType.hpp"
#include "system/Path.hpp"

#include <vector>

std::vector<AllocatedPath>
DownloadFilePicker(FileType file_type);

/**
 * Pick one or more repository files.  Groups by country/area when
 * more than one area is present.  Returns the chosen files (not
 * downloaded).  Empty if the user cancelled.
 */
std::vector<AvailableFile>
SelectAvailableFiles(std::vector<AvailableFile> files);
