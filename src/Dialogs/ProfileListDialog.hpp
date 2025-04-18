// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The XCSoar Project

#pragma once

class Path;
class AllocatedPath;

void
ProfileListDialog();

/**
 * Let the user select a profile file.  Returns the absolute path of
 * the selected file or an empty string if the user has cancelled the
 * dialog.
 */
AllocatedPath
SelectProfileDialog(Path selected_path);
