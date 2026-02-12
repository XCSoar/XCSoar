// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The XCSoar Project

#pragma once

#include "system/Path.hpp"

class AllocatedPath;

/**
 * Download a file showing a modal #ProgressDialog with a cancel
 * button.  The message shown in the dialog body is the filename
 * portion of #base.
 *
 * Throws on error.
 *
 * @param caption the dialog window caption
 * @param uri the URI to download
 * @param base the local filename to store the download as
 * @return the full local path on success, nullptr on cancel
 */
AllocatedPath
DownloadFileModal(const char *caption, const char *uri, const char *base);
