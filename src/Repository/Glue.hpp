// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The XCSoar Project

#pragma once

/**
 * Download the repository file.
 *
 * @param force if true, then download it even when this library
 * believes it is still fresh
 */
void
EnqueueRepositoryDownload(bool force=false);
