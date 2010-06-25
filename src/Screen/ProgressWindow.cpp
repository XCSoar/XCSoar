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

#include "Screen/ProgressWindow.hpp"
#include "Screen/BitmapCanvas.hpp"
#include "Version.hpp"
#include "resource.h"

#include <algorithm>

using std::min;

ProgressWindow::ProgressWindow(ContainerWindow &parent)
  :background_color(Color::WHITE),
   background_brush(background_color),
   position(0)
{
  RECT rc = parent.get_client_rect();
  set(parent, rc.left, rc.top, rc.right, rc.bottom);

  unsigned width = rc.right - rc.left, height = rc.bottom - rc.top;

  logo.load(width >= 280 && height >= 220 ? IDB_SWIFT : IDB_SWIFT2);

  VirtualCanvas canvas(1, 1);
  unsigned text_height = canvas.text_height(_T("W"));
  unsigned progress_size = text_height * 3 / 2;

  TextWindowStyle version_style;
  version_style.left();
  version.set(*this, XCSoar_ProductToken,
              0, 0, width - progress_size * 2, text_height, version_style);

  TextWindowStyle message_style;
  message_style.center();

  ProgressBarStyle pb_style;
  pb_style.border();

  if (width < height) {
    /* portrait */

    message.set(*this, NULL, 0, height - text_height - progress_size - 8,
                width, text_height, message_style);
    progress_bar.set(*this, 10, height - progress_size - 10,
                     width - 20, progress_size, pb_style);
  } else {
    /* landscape */

    message.set(*this, NULL, 10, height - text_height - 10,
                width - progress_size - 20, text_height, message_style);

    pb_style.vertical();
    pb_style.smooth();
    progress_bar.set(*this, width - progress_size - 10, 10,
                     progress_size, height - 20, pb_style);
  }

  version.install_wndproc(); // needed for on_color()
  message.install_wndproc(); // needed for on_color()

  set_range(0, 1000);
  set_step(50);

  bring_to_top();
  update();
}

void
ProgressWindow::set_message(const TCHAR *text)
{
  assert_none_locked();
  assert_thread();

  message.set_text(text);
}

void
ProgressWindow::set_range(unsigned min_value, unsigned max_value)
{
  progress_bar.set_range(min_value, max_value);
}

void
ProgressWindow::set_step(unsigned size)
{
  progress_bar.set_step(size);
}

void
ProgressWindow::set_pos(unsigned value)
{
  assert_none_locked();
  assert_thread();

  if (value == position)
    return;

  position = value;
  progress_bar.set_position(value);
}

void
ProgressWindow::step()
{
  progress_bar.step();
}

bool
ProgressWindow::on_erase(Canvas &canvas)
{
  canvas.clear(background_brush);
  return true;
}

void
ProgressWindow::on_paint(Canvas &canvas)
{
  BitmapCanvas bitmap_canvas(canvas, logo);

  int window_width = canvas.get_width();
  int window_height = canvas.get_height();
  int bitmap_width = bitmap_canvas.get_width();
  int bitmap_height = bitmap_canvas.get_height();

  int scale = min((window_width - 20) / bitmap_width,
                  (window_height - 20) / bitmap_height);
  if (scale > 1) {
    int dest_width = bitmap_width * scale;
    int dest_height = bitmap_height * scale;

    canvas.stretch((window_width - dest_width) / 2,
                   (window_height - dest_height) / 2,
                   dest_width, dest_height,
                   bitmap_canvas, 0, 0,
                   bitmap_width, bitmap_height);
  } else
    canvas.copy((window_width - bitmap_width) / 2,
                (window_height - bitmap_height) / 2,
                bitmap_width, bitmap_height,
                bitmap_canvas, 0, 0);
}

Brush *
ProgressWindow::on_color(Window &window, Canvas &canvas)
{
  canvas.set_text_color(Color::BLACK);
  canvas.set_background_color(background_color);
  return &background_brush;
}
