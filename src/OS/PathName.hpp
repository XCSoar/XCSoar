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

#ifndef OS_PATH_HPP
#define OS_PATH_HPP

#include "Compiler.h"

#include <tchar.h>

/**
 * Replaces the "base name" of the specified path, i.e. the portion
 * after the last path separator.  If the input path does not contain
 * a directory name, the whole string is replaced.
 *
 * @param the input and output buffer
 * @param new_base the new base name to be copied to #path
 */
void
ReplaceBaseName(TCHAR *path, const TCHAR *new_base);

/**
 * Checks whether the given filename matches the given extension
 * @param filename Filename to check
 * @param extension Extension to check against (e.g. .xcm)
 * @return True if filename matches the given extension, False otherwise
 */
gcc_pure
bool MatchesExtension(const TCHAR *filename, const TCHAR *extension);

#ifdef _UNICODE

/**
 * Checks whether the given filename matches the given extension
 * @param filename Filename to check
 * @param extension Extension to check against (e.g. .xcm)
 * @return True if filename matches the given extension, False otherwise
 */
gcc_pure
bool MatchesExtension(const char *filename, const char *extension);

#endif

#endif
