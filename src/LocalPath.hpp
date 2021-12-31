/*
Copyright_License {

  XCSoar Glide Computer - http://www.xcsoar.org/
  Copyright (C) 2000-2021 The XCSoar Project
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
DeinitialiseDataPath() noexcept;

/**
 * Create the primary data path;
 */
void
CreateDataPath();

/**
 * Overrides the detected primary data path.
 */
void
SetPrimaryDataPath(Path path) noexcept;

/**
 * Returns the absolute path of the primary data directory.
 */
Path
GetPrimaryDataPath() noexcept;

/**
 * Gives the position of an XCSoar data file within the particular file
 * system of this host.
 * @param file The name of the file in question. Should not be equal to 
 *             'nullptr'.
 * @return The fully qualified path of file.
 */
AllocatedPath
LocalPath(Path file) noexcept;

AllocatedPath
LocalPath(const TCHAR *file) noexcept;

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
[[gnu::pure]]
Path
RelativePath(Path path) noexcept;

/**
 * Converts a file path by replacing %LOCAL_PATH% with the full pathname to
 * the XCSoarData folder
 * @param filein Pointer to the string to convert
 */
[[gnu::pure]]
AllocatedPath
ExpandLocalPath(Path src) noexcept;

/**
 * Converts a file path from full pathname to a shorter version with the
 * XCSoarData folder replaced by %LOCAL_PATH%
 * @param filein Pointer to the string to convert
 * @return the new path or nullptr if the given path cannot be contracted
 */
[[gnu::pure]]
AllocatedPath
ContractLocalPath(Path src) noexcept;

void VisitDataFiles(const TCHAR* filter, File::Visitor &visitor);

[[gnu::pure]]
Path
GetCachePath() noexcept;

[[gnu::pure]]
AllocatedPath
MakeCacheDirectory(const TCHAR *name) noexcept;

#endif
