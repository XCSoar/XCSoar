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

#ifndef XCSOAR_PROFILE_HPP
#define XCSOAR_PROFILE_HPP

// IWYU pragma: begin_exports
#include "Profile/ProfileKeys.hpp"
#include "Profile/ProfileMap.hpp"
// IWYU pragma: end_exports

#include "Compiler.h"

class Path;
class AllocatedPath;

namespace Profile
{
  /**
   * Returns the absolute path of the current profile file.
   */
  gcc_pure
  Path GetPath();

  /**
   * Loads the profile files
   */
  void Load();
  /**
   * Loads the given profile file
   */
  void LoadFile(Path path);

  /**
   * Saves the profile into the profile files
   *
   * Errors will be caught and logged.
   */
  void Save() noexcept;
  /**
   * Saves the profile into the given profile file
   */
  void SaveFile(Path path);

  /**
   * Sets the profile files to load when calling Load()
   * @param override nullptr or file to load when calling Load()
   */
  void SetFiles(Path override_path);

  /**
   * Reads a configured path from the profile, and expands it with
   * ExpandLocalPath().
   *
   * @param value a buffer which can store at least MAX_PATH
   * characters
   */
  gcc_pure
  AllocatedPath GetPath(const char *key);

  void SetPath(const char *key, Path value);

  gcc_pure
  bool GetPathIsEqual(const char *key, Path value);
};

#endif
