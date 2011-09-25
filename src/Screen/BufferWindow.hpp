/*
Copyright_License {

  XCSoar Glide Computer - http://www.xcsoar.org/
  Copyright (C) 2000-2011 The XCSoar Project
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

#ifndef XCSOAR_SCREEN_BUFFER_WINDOW_HXX
#define XCSOAR_SCREEN_BUFFER_WINDOW_HXX

#include "Screen/PaintWindow.hpp"

#ifndef ENABLE_OPENGL
#include "Screen/BufferCanvas.hpp"
#endif

/**
 * A #PaintWindow with buffered painting, to avoid flickering.
 */
class BufferWindow : public PaintWindow {
#ifndef ENABLE_OPENGL
private:
  BufferCanvas buffer;

  /**
   * Is the buffer dirty, i.e. does it need a full repaint with
   * on_paint_buffer()?
   */
  bool dirty;

public:
  void invalidate() {
    dirty = true;
    PaintWindow::invalidate();
  }
#endif

protected:
  /**
   * Determines whether this BufferWindow maintains a persistent
   * buffer which allows incremental drawing in each frame.
   */
  static bool is_persistent() {
#ifdef ENABLE_OPENGL
    /* on OpenGL, there is no per-window buffer, each frame needs to
       be redrawn from scratch */
    return false;
#else
    return true;
#endif
  }

protected:
#ifndef ENABLE_OPENGL
  virtual bool on_create();
  virtual bool on_destroy();

  virtual bool on_resize(UPixelScalar width, UPixelScalar height);
#endif

  virtual void on_paint(Canvas &canvas);

  virtual void on_paint_buffer(Canvas &canvas) = 0;
};

#endif
