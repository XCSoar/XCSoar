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

/*
 * Load an image from a resource or a file and exit.
 *
 */

#define ENABLE_SCREEN
#define ENABLE_CMDLINE
#define USAGE "{PATH|ID}"

#include "Main.hpp"
#include "Screen/Bitmap.hpp"
#include "Util/NumberParser.hpp"
#include "Util/StringUtil.hpp"

static tstring path;
static unsigned id;

static void
ParseCommandLine(Args &args)
{
  path = args.ExpectNextT();

  TCHAR *endptr;
  id = ParseUnsigned(path.c_str(), &endptr);
  if (!StringIsEmpty(endptr))
    id = 0;
}

static void
Main()
{
  Bitmap bitmap;
  bool success = id != 0
    ? bitmap.Load(id)
    : bitmap.LoadFile(path.c_str());
  if (!success)
    fprintf(stderr, "Failed to load image\n");
}
