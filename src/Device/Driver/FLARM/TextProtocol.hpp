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

#ifndef XCSOAR_FLARM_TEXT_PROTOCOL_HPP
#define XCSOAR_FLARM_TEXT_PROTOCOL_HPP

#include "util/Compiler.h"

/**
 * Copy a string, ignoring all characters that are illegal in a
 * setting value.  There is no buffer overflow check; the destination
 * buffer must be large enough to fit all of the source string
 * (worst-case).
 */
char *
CopyCleanFlarmString(char *gcc_restrict dest, const char *gcc_restrict src);

#endif
