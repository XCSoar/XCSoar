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

#include "AutoFont.hpp"
#include "FontDescription.hpp"
#include "ui/canvas/Font.hpp"

#include <algorithm>

void
AutoSizeFont(FontDescription &d, unsigned width, const TCHAR *text)
{
  // JMW algorithm to auto-size info window font.
  // this is still required in case title font property doesn't exist.

  /* reasonable start value (ignoring the original input value) */
  d.SetHeight(std::max(10u, unsigned(width) / 4u));

  Font font;
  font.Load(d);

  /* double the font size until it is large enough */

  PixelSize tsize = font.TextSize(text);
  while (tsize.width < width) {
    d.SetHeight(d.GetHeight() * 2);
    font.Load(d);

    tsize = font.TextSize(text);
  }

  /* decrease font size until it fits exactly */

  do {
    if (d.GetHeight() <= 6)
      /* this is the lower bound; don't go smaller than that, or else
         the size will underflow eventually */
      break;

    d.SetHeight(d.GetHeight() - 1);

    Font font;
    font.Load(d);

    tsize = font.TextSize(text);
  } while (tsize.width > width);

  d.SetHeight(d.GetHeight() + 1);
}
