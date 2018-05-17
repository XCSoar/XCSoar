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

#include "Init.hpp"

#if defined(__clang__) && defined(__arm__)
/* work around warning: 'register' storage class specifier is
   deprecated */
#define register
#endif

#include <ft2build.h>
#include FT_FREETYPE_H

#include <stdio.h>

#ifdef KOBO
bool FreeType::mono = true;
#endif

static FT_Library ft_library;

void
FreeType::Initialise()
{
  FT_Error error = FT_Init_FreeType(&ft_library);
  if (error) {
    fprintf(stderr, "FT_Init_FreeType() failed\n");
    abort();
  }
}

void
FreeType::Deinitialise()
{
  FT_Done_FreeType(ft_library);
}

FT_Face
FreeType::Load(const char *path)
{
  FT_Face face;
  FT_Error error = FT_New_Face(ft_library, path, 0, &face);
  if (error)
    return nullptr;

  return face;
}
