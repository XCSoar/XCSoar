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

#ifndef XCSOAR_FORM_DRAW_HPP
#define XCSOAR_FORM_DRAW_HPP

#include "Screen/PaintWindow.hpp"

#include <functional>

class ContainerWindow;

/**
 * This class is used for creating custom drawn content.
 * It is based on the WindowControl class.
 */
class WndOwnerDrawFrame : public PaintWindow {
public:
  template<typename CB>
  void Create(ContainerWindow &parent,
              PixelRect rc, const WindowStyle style,
              CB &&_paint) {
    mOnPaintCallback = std::move(_paint);
    PaintWindow::Create(parent, rc, style);
  }

protected:
  /**
   * The callback function for painting the content of the control
   * @see SetOnPaintNotify()
   */
  std::function<void(Canvas &canvas, const PixelRect &rc)> mOnPaintCallback;

  /** from class PaintWindow */
  void OnPaint(Canvas &canvas) override;
};

#endif
