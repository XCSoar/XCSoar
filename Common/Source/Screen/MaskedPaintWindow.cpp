/*
Copyright_License {

  XCSoar Glide Computer - http://www.xcsoar.org/
  Copyright (C) 2000 - 2009

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

#include "Screen/MaskedPaintWindow.hpp"
#include "Screen/BitmapCanvas.hpp"
#include "Screen/Bitmap.hpp"
#include "Screen/Canvas.hpp"
#include "Screen/Window.hpp"
#include "InfoBoxLayout.h"
#include "options.h" /* for IBLSCALE() */

#include <assert.h>

void
MaskedPaintWindow::set(ContainerWindow *parent,
                 int left, int top, unsigned width, unsigned height,
                 bool center, bool notify, bool show,
                 bool tabstop, bool border)
{
  mask_canvas.reset();
  PaintWindow::set(parent, left, top, width, height, center, notify, show,
		   tabstop, border);
  if (!mask_canvas.defined())
    mask_canvas.set(get_canvas());
}

void
MaskedPaintWindow::resize(unsigned width, unsigned height)
{
  PaintWindow::resize(width, height);
  mask_canvas.resize(width, height);
}

void
MaskedPaintWindow::reset()
{
  mask_canvas.reset();
  PaintWindow::reset();
}

bool
MaskedPaintWindow::on_create()
{
  assert(!mask_canvas.defined());

  if (!PaintWindow::on_create())
    return false;

  mask_canvas.set(get_canvas());
  return true;
}


void MaskedPaintWindow::draw_masked_bitmap(Canvas &canvas, const Bitmap &bitmap, 
					   const int x, const int y, 
					   const unsigned src_width, 
					   const unsigned src_height, 
					   bool centered)
{
  int x_offset = centered? IBLSCALE(src_width)/2 : 0;
  int y_offset = centered? IBLSCALE(src_height)/2 : 0;

  mask_canvas.clear();
  mask_canvas.background_opaque();
  mask_canvas.set_text_color(Color(0xff,0xff, 0xff));
  mask_canvas.select(bitmap);
  canvas.scale_or_and(x-x_offset, y-y_offset, mask_canvas, src_width, src_height);
}

void MaskedPaintWindow::draw_bitmap(Canvas &canvas, const Bitmap &bitmap, 
				    const int x, const int y, 
				    const unsigned src_x_offset, 
				    const unsigned src_y_offset, 
				    const unsigned src_width, 
				    const unsigned src_height, 
				    bool centered)
{
  int x_offset = centered? IBLSCALE(src_width)/2 : 0;
  int y_offset = centered? IBLSCALE(src_height)/2 : 0;

  mask_canvas.clear();
  mask_canvas.background_opaque();
  mask_canvas.set_text_color(Color(0xff,0xff, 0xff));
  mask_canvas.select(bitmap);
  canvas.scale_copy(x-x_offset, y-y_offset, 
		    mask_canvas, 
		    src_x_offset, src_y_offset, 
		    src_width, src_height);
}
