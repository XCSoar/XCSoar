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

#ifndef XCSOAR_LOCAL_PATH_HPP
#define XCSOAR_LOCAL_PATH_HPP

#include "Compiler.h"

#include <tchar.h>

class Path;
class AllocatedPath;

namespace File {
  class Visitor;
}

/**
 * Determine the data path.
 *
 * @return true on success, false if no data path could be found
 */
bool
InitialiseDataPath();

/**
 * Release resources obtained by InitialiseDataPath().
 */
void
DeinitialiseDataPath();

/**
 * Create the primary data path;
 */
void
CreateDataPath();

/**
 * Overrides the detected primary data path.
 */
void
SetPrimaryDataPath(Path path);

/**
 * Returns the absolute path of the primary data directory.
 */
Path
GetPrimaryDataPath();

/**
 * Returns the path of the XCSoarData folder, optionally including
 * the given file name
 * @param buffer Output buffer
 * @param file optional filename to include in the output
 */
AllocatedPath
LocalPath(Path file);

AllocatedPath
LocalPath(const TCHAR *file);

/**
 * Create a subdirectory of XCSoarData and return its absolute path.
 */
AllocatedPath
MakeLocalPath(const TCHAR *name);

/**
 * Return the portion of the specified path that is relative to the
 * primary data path.  Returns nullptr on failure (if the path is not
 * inside the primary data path).
 */
gcc_pure
Path
RelativePath(Path path);

/**
 * Converts a file path by replacing %LOCAL_PATH% with the full pathname to
 * the XCSoarData folder
 * @param filein Pointer to the string to convert
 */
gcc_pure
AllocatedPath
ExpandLocalPath(Path src);

/**
 * Converts a file path from full pathname to a shorter version with the
 * XCSoarData folder replaced by %LOCAL_PATH%
 * @param filein Pointer to the string to convert
 * @return the new path or nullptr if the given path cannot be contracted
 */
gcc_pure
AllocatedPath
ContractLocalPath(Path src);

void VisitDataFiles(const TCHAR* filter, File::Visitor &visitor);

#endif
