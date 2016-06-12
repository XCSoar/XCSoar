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

#ifndef XCSOAR_SCREEN_BUFFER_WINDOW_HXX
#define XCSOAR_SCREEN_BUFFER_WINDOW_HXX

#include "Screen/PaintWindow.hpp"
#include "Screen/BufferCanvas.hpp"

/**
 * A #PaintWindow with buffered painting, to avoid flickering.
 */
class BufferWindow : public PaintWindow {
  BufferCanvas buffer;

  /**
   * Is the buffer dirty, i.e. does it need a full repaint with
   * OnPaintBuffer()?
   */
  bool dirty;

public:
  void Invalidate()
#ifndef USE_WINUSER
    override
#endif
  {
    dirty = true;
    PaintWindow::Invalidate();
  }

  void Invalidate(const PixelRect &rect) {
    dirty = true;
    PaintWindow::Invalidate(rect);
  }

protected:
  /**
   * Determines whether this BufferWindow maintains a persistent
   * buffer which allows incremental drawing in each frame.
   */
  static constexpr bool IsPersistent() {
    return true;
  }

protected:
  /* virtual methods from class Window */
#ifndef ENABLE_OPENGL
  void OnCreate() override;
  void OnDestroy() override;
#endif

  void OnResize(PixelSize new_size) override;

  /* virtual methods from class PaintWindow */
  void OnPaint(Canvas &canvas) override;

  /* our virtual methods */
  virtual void OnPaintBuffer(Canvas &canvas) = 0;
};

#endif
