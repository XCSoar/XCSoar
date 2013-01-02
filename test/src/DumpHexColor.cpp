/*
Copyright_License {

  XCSoar Glide Computer - http://www.xcsoar.org/
  Copyright (C) 2000-2013 The XCSoar Project
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

#include "Formatter/HexColor.hpp"
#include "Screen/Color.hpp"
#include "OS/Args.hpp"
#include "Util/Macros.hpp"

#include <stdio.h>
#include <stdlib.h>

int main(int argc, char **argv)
{
  Args args(argc, argv, "COLOR ...");
  tstring s = args.ExpectNextT();
  while (true) {
    Color color;
    if (!ParseHexColor(s.c_str(), color)) {
      _ftprintf(stderr, _T("Failed to parse '%s'\n"), s.c_str());
      return EXIT_FAILURE;
    }

    TCHAR buffer[32];
    FormatHexColor(buffer, ARRAY_SIZE(buffer), color);

    _tprintf(_T("%s -> %s\n"), s.c_str(), buffer);

    if (args.IsEmpty())
      break;

    s = args.ExpectNextT();
  }

  return EXIT_SUCCESS;
}
