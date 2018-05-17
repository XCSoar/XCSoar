/*
Copyright_License {

  XCSoar Glide Computer - http://www.xcsoar.org/
  Copyright (C) 2000-2016 The XCSoar Project
  A detailed list of copyright holders can be found in the file "AUTHORS".

  This program is free software; you can redistribute it and/or
  modify it under the terms of the GNU General Public License
  as published by the Free Software Foundation; either version 2
  of the License, or (at your option) any later version.

  This program is distributed in the hope that it will be useful,
  but WITHOUT ANY WARRANTY; without even the implied warranty of
  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
  GNU General Public License for more details.

  You should have received a copy of the GNU General Public License
  along with this program; if not, write to the Free Software
  Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA  02111-1307, USA.
}
*/

#ifndef XCSOAR_DIALOGS_FILE_PICKER_HPP
#define XCSOAR_DIALOGS_FILE_PICKER_HPP

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

#endif
