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

#ifdef HAVE_POSIX
#include "Device/Port/TTYEnumerator.hpp"
#endif

#include <stdio.h>
#include <stdlib.h>

int main(int argc, char **argv)
{
  bool implemented = false, success = false;

#ifdef HAVE_POSIX
  implemented = true;

  TTYEnumerator te;
  if (!te.HasFailed()) {
    success = true;

    const char *path;
    while ((path = te.Next()) != nullptr)
      printf("%s\n", path);
  } else
    fprintf(stderr, "Failed to enumerate TTY ports\n");
#endif

  if (!implemented)
    fprintf(stderr, "Port enumeration not implemented on this target\n");

  return success ? EXIT_SUCCESS : EXIT_FAILURE;
}
