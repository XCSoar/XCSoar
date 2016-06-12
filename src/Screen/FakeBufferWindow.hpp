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

#ifndef XCSOAR_SCREEN_FAKE_BUFFER_WINDOW_HXX
#define XCSOAR_SCREEN_FAKE_BUFFER_WINDOW_HXX

#include "PaintWindow.hpp"

/**
 * Emulation of the #BufferWindow API without actually buffering.
 */
class FakeBufferWindow : public PaintWindow {
protected:
  /**
   * Determines whether this class maintains a persistent buffer which
   * allows incremental drawing in each frame.
   */
  static constexpr bool IsPersistent() {
    return false;
  }

  virtual void OnPaintBuffer(Canvas &canvas) = 0;

  /* virtual methods from class Window */
  void OnPaint(Canvas &canvas) override {
    OnPaintBuffer(canvas);
  }
};

#endif
