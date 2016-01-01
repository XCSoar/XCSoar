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
#include "Util/StringCompare.hxx"
#include "ResourceId.hpp"

static AllocatedPath path = nullptr;

#ifdef USE_GDI
static ResourceId id = ResourceId::Null();
#endif

static void
ParseCommandLine(Args &args)
{
  path = args.ExpectNextPath();

#ifdef USE_GDI
  TCHAR *endptr;
  unsigned _id = ParseUnsigned(path.c_str(), &endptr);
  if (StringIsEmpty(endptr))
    id = ResourceId(_id);
#endif
}

static void
Main()
{
  Bitmap bitmap;
  bool success =
#ifdef USE_GDI
    id.IsDefined()
    ? bitmap.Load(id)
    :
#endif
    bitmap.LoadFile(path);
  if (!success)
    fprintf(stderr, "Failed to load image\n");
}
