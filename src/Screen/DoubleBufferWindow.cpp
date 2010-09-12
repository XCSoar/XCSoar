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

#include "Screen/DoubleBufferWindow.hpp"
#include "Screen/WindowCanvas.hpp"

bool
DoubleBufferWindow::on_create()
{
  if (!PaintWindow::on_create())
    return false;

  WindowCanvas a_canvas(*this);
  buffers[0].set(a_canvas);
  buffers[1].set(a_canvas);
  return true;
}

bool
DoubleBufferWindow::on_destroy()
{
  PaintWindow::on_destroy();

  buffers[0].reset();
  buffers[1].reset();
  return true;
}

void
DoubleBufferWindow::flip()
{
  /* enable the drawing buffer */
  mutex.Lock();
  current ^= 1;
  mutex.Unlock();

  /* commit the finished buffer to the screen (asynchronously) */
  invalidate();

  /* grow the current buffer, just in case the window has been
     resized */
  buffers[current].grow(get_width(), get_height());
}

void
DoubleBufferWindow::on_paint(Canvas &canvas)
{
  ScopeLock protect(mutex);
  canvas.copy(get_visible_canvas());
}
