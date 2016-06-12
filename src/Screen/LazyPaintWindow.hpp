/*
Copyright_License {

  XCSoar Glide Computer - http://www.xcsoar.org/
  Copyright (C) 2000-2015 The XCSoar Project
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

#ifndef XCSOAR_SCREEN_LAZY_PAINT_WINDOW_HXX
#define XCSOAR_SCREEN_LAZY_PAINT_WINDOW_HXX

#ifdef USE_GDI

#include "FakeBufferWindow.hpp"

class LazyPaintWindow : public FakeBufferWindow {
};

#else

#include "BufferWindow.hpp"

/**
 * A #PaintWindow implementation which avoids calling OnPaint() unless
 * Invalidate() has been called explicitly.  It will try to avoid
 * OnPaint() if the old screen contents are still available (which is
 * only possible with GDI).  Implementations which require XCSoar to
 * redraw the whole screen at a time (like OpenGL) need a buffer.
 */
class LazyPaintWindow : public BufferWindow {
};

#endif

#endif
