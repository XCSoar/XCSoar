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

#ifndef XCSOAR_SCREEN_MASKED_PAINT_WINDOW_HXX
#define XCSOAR_SCREEN_MASKED_PAINT_WINDOW_HXX

#include "Screen/PaintWindow.hpp"
#include "Screen/BitmapCanvas.hpp"

/**
 * A #Window implementation for custom drawing.  Call get_canvas()
 * whenever you want to draw something.  This also has a mask buffer
 * to offer masked draws
 */
class MaskedPaintWindow : public PaintWindow {
private:
  BitmapCanvas mask_canvas;

public:
  void set(ContainerWindow &parent, LPCTSTR cls,
           int left, int top, unsigned width, unsigned height) {
    PaintWindow::set(parent, cls, left, top, width, height);
  }
  void set(ContainerWindow *parent,
           int left, int top, unsigned width, unsigned height,
           bool center = false, bool notify = false, bool show = true,
           bool tabstop = false, bool border = false);

  void reset();

#ifndef ENABLE_SDL
  void resize(unsigned width, unsigned height);
#endif /* !ENABLE_SDL */

  void draw_masked_bitmap(Canvas &canvas, const Bitmap &bitmap, 
			  const int x, const int y,
			  const unsigned src_width, 
			  const unsigned src_height, 
			  bool centered=true);

  void draw_bitmap(Canvas &canvas, const Bitmap &bitmap, 
		   const int x, const int y, 
		   const unsigned src_x_offset, 
		   const unsigned src_y_offset, 
		   const unsigned src_width, 
		   const unsigned src_height, 
		   bool centered=true);

protected:
  virtual bool on_create();
};

#endif
