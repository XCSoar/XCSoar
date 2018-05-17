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

#ifndef XCSOAR_LOGO_VIEW_HPP
#define XCSOAR_LOGO_VIEW_HPP

#include "Screen/Bitmap.hpp"
#include "Screen/Font.hpp"

class Canvas;

class LogoView {
  Bitmap logo, big_logo, title, big_title;

#ifndef USE_GDI
  Font font;
#endif

public:
  LogoView();

  /**
   * Draws the XCSoar logo and the version number into the Canvas, with
   * a white background.
   *
   * @param canvas the Canvas to draw on
   * @param rc the region within the Canvas to draw into
   */
  void draw(Canvas &canvas, const PixelRect &rc);
};

#endif
