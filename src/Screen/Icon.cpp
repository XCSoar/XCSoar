/*
Copyright_License {

  XCSoar Glide Computer - http://www.xcsoar.org/
  Copyright (C) 2000, 2001, 2002, 2003, 2004, 2005, 2006, 2007, 2008, 2009

	M Roberts (original release)
	Robin Birch <robinb@ruffnready.co.uk>
	Samuel Gisiger <samuel.gisiger@triadis.ch>
	Jeff Goodenough <jeff@enborne.f2s.com>
	Alastair Harrison <aharrison@magic.force9.co.uk>
	Scott Penrose <scottp@dd.com.au>
	John Wharington <jwharington@gmail.com>
	Lars H <lars_hn@hotmail.com>
	Rob Dunning <rob@raspberryridgesheepfarm.com>
	Russell King <rmk@arm.linux.org.uk>
	Paolo Ventafridda <coolwind@email.it>
	Tobias Lohner <tobias@lohner-net.de>
	Mirek Jezek <mjezek@ipplc.cz>
	Max Kellermann <max@duempel.org>
	Tobias Bieniek <tobias.bieniek@gmx.de>

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

#include "Screen/Icon.hpp"
#include "Screen/Layout.hpp"
#include "Screen/Canvas.hpp"
#include "Screen/BitmapCanvas.hpp"

void
MaskedIcon::load(unsigned id, bool center)
{
  bitmap.load(id);

  size = bitmap.get_size();
  /* left half is mask, right half is icon */
  size.cx /= 2;

  SIZE screen_size;
  screen_size.cx = Layout::FastScale(size.cx);
  screen_size.cy = Layout::FastScale(size.cy);

  if (center) {
    origin.x = screen_size.cx / 2;
    origin.y = screen_size.cy / 2;
  } else
    origin.x = origin.y = 0;
}

void
MaskedIcon::draw(Canvas &canvas, BitmapCanvas &bitmap_canvas, int x, int y) const
{
  bitmap_canvas.background_opaque();
  bitmap_canvas.set_text_color(Color::WHITE);
  bitmap_canvas.select(bitmap);

  canvas.scale_or_and(x - origin.x, y - origin.y,
                      bitmap_canvas, size.cx, size.cy);
}
