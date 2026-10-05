// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The XCSoar Project

#pragma once

#include "Repository/FileType.hpp"

#include <vector>

class AllocatedPath;

/**
 * Let the user tick repository files and download them.
 *
 * @return the local paths of the files that finished downloading,
 * in the order they were fetched; empty if nothing was downloaded
 */
std::vector<AllocatedPath>
DownloadFilePicker(FileType file_type);
