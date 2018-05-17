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

#ifndef XCSOAR_SCREEN_FREETYPE_INIT_HPP
#define XCSOAR_SCREEN_FREETYPE_INIT_HPP

typedef struct FT_FaceRec_ *FT_Face;

namespace FreeType {
#ifdef KOBO
  /**
   * Are we using monochrome font rendering mode on the Kobo?  This
   * can be disabled for some situations; see
   * TopCanvas::SetEnableDither().
   */
  extern bool mono;
#endif

  void Initialise();
  void Deinitialise();

  FT_Face Load(const char *path);
};

#endif
