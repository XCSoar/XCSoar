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

#ifndef XCSOAR_UTIL_ESCAPE_BACKSLASH_HPP
#define XCSOAR_UTIL_ESCAPE_BACKSLASH_HPP

#include "Compiler.h"

#include <tchar.h>

/**
 * Parses the special characters (cr, lf, back slash) in the old_string and
 * returns the parsed new_string
 * @param old_string The old string with (or without) special characters
 * @return The new parsed string
 */
gcc_nonnull_all
TCHAR *
UnescapeBackslash(const TCHAR *old_string);

#endif
