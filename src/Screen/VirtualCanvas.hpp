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

#ifndef XCSOAR_SCREEN_VIRTUAL_CANVAS_HPP
#define XCSOAR_SCREEN_VIRTUAL_CANVAS_HPP

#include "Screen/Canvas.hpp"

/**
 * A #Canvas implementation which draws to an off-screen surface.
 * This is an abstract class; see #BufferCanvas for
 * a concrete implementation.
 */
class VirtualCanvas : public Canvas {
public:
  VirtualCanvas() = default;
  VirtualCanvas(PixelSize new_size);
  VirtualCanvas(const Canvas &canvas, PixelSize new_size);

  ~VirtualCanvas() {
    Destroy();
  }

  void Create(PixelSize new_size);

  void Create(const Canvas &canvas, PixelSize new_size);

  void Create(const Canvas &canvas) {
    Create(canvas, canvas.GetSize());
  }

  void Destroy();

#ifdef USE_MEMORY_CANVAS
  void Resize(PixelSize new_size) {
    if (new_size != GetSize())
      Create(*this, new_size);
  }

  void Grow(PixelSize new_size);
#endif
};

#endif
