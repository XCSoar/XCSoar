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

#include "TTYEnumerator.hpp"
#include "Compiler.h"

#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>

gcc_pure
static bool
CheckTTYName(const char *name)
{
  /* filter "/dev/tty*" */
  if (memcmp(name, "tty", 3) == 0) {
    /* ignore virtual internal ports on Mac OS X (and probably other
       BSDs) */
    if (name[3] >= 'p' && name[3] <= 'w')
      return false;

    /* filter out "/dev/tty0", ... (valid integer after "tty") */
    char *endptr;
    strtoul(name + 3, &endptr, 10);
    return *endptr != 0;
  } else if (memcmp(name, "rfcomm", 6) == 0)
    return true;
  else
    return false;
}

const char *
TTYEnumerator::Next()
{
  struct dirent *ent;
  while ((ent = readdir(dir)) != nullptr) {
    if (!CheckTTYName(ent->d_name))
      continue;

    if (snprintf(path, sizeof(path), "/dev/%s", ent->d_name) >= (int)sizeof(path))
      /* truncated - ignore */
      continue;

    if (access(path, R_OK|W_OK) == 0 && access(path, X_OK) < 0)
      return path;
  }

  return nullptr;
}
