/*
Copyright_License {

  XCSoar Glide Computer - http://www.xcsoar.org/
  Copyright (C) 2000-2012 The XCSoar Project
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

#ifndef XCSOAR_SCREEN_DOUBLE_BUFFER_WINDOW_HXX
#define XCSOAR_SCREEN_DOUBLE_BUFFER_WINDOW_HXX

#include "Screen/PaintWindow.hpp"
#include "Screen/BufferCanvas.hpp"
#include "Thread/Mutex.hpp"

/**
 * A #PaintWindow with double buffered painting, to avoid flickering.
 * Having two buffers, one thread may render, while the main threadd
 * copies the other buffer to the screen.
 */
class DoubleBufferWindow : public PaintWindow {
#ifdef ENABLE_OPENGL
  /* on OpenGL, there is no DrawThread, and we use only one buffer to
     cache the painted window, to reduce CPU/GPU usage for new
     frames */
  BufferCanvas buffer;

  /**
   * Is the buffer dirty, i.e. does it need a full repaint with
   * OnPaintBuffer()?
   */
  bool dirty;

public:
  void Invalidate() {
    dirty = true;
    PaintWindow::Invalidate();
  }

protected:
  virtual void OnCreate() gcc_override;
  virtual void OnDestroy() gcc_override;
  virtual void OnResize(UPixelScalar width, UPixelScalar height) gcc_override;

#else
  BufferCanvas buffers[2];

  /**
   * The buffer currently drawn into.  This buffer may only be
   * accessed by the drawing thread.  The other buffer (current^1) may
   * only be accessed by the main thread.
   */
  unsigned current;

protected:
  /**
   * This mutex protects the variable "current".
   */
  Mutex mutex;

public:
  DoubleBufferWindow()
    :current(0) {}

private:
  /**
   * Returns the Canvas which is currently used for rendering.  This
   * method may only be called within the drawing thread.
   */
  Canvas &get_canvas() {
    return buffers[current];
  }

  /**
   * Marks the hidden Canvas as "done" and schedules it for painting
   * to the Window.
   */
  void flip();

protected:
  /**
   * Returns the Canvas which is currently visible.  A call to this
   * method must be protected with the Mutex.
   */
  const Canvas &get_visible_canvas() const {
    return buffers[current ^ 1];
  }

protected:
  virtual void OnCreate() gcc_override;
  virtual void OnDestroy() gcc_override;
#endif

protected:
  virtual void OnPaint(Canvas &canvas) gcc_override;
  virtual void OnPaintBuffer(Canvas &canvas) = 0;

public:
  void repaint() {
#ifndef ENABLE_OPENGL
    OnPaintBuffer(get_canvas());
    flip();
#else
    Invalidate();
#endif
  }
};

#endif
