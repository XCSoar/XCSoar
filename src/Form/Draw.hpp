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

#ifndef XCSOAR_FORM_DRAW_HPP
#define XCSOAR_FORM_DRAW_HPP

#include "Form/Control.hpp"

class ContainerWindow;

/**
 * This class is used for creating custom drawn content.
 * It is based on the WindowControl class.
 */
class WndOwnerDrawFrame : public PaintWindow {
public:
  typedef void (*OnPaintCallback_t)(WndOwnerDrawFrame *Sender, Canvas &canvas);
  typedef bool (*OnMouseDownCallback_t)(WndOwnerDrawFrame *Sender,
                                        PixelScalar x, PixelScalar y);

public:
  WndOwnerDrawFrame(ContainerWindow &parent,
                    PixelScalar x, PixelScalar y,
                    UPixelScalar width, UPixelScalar height,
                    const WindowStyle style,
                    OnPaintCallback_t OnPaintCallback);

  /**
   * Sets the callback which actually paints the window.  The
   * background is cleared before, and all configured fonts and colors
   * have been set in the #Canvas.
   */
  void SetOnPaintNotify(OnPaintCallback_t OnPaintCallback) {
    mOnPaintCallback = OnPaintCallback;
  }

  /**
   * Sets the callback which is called when the mouse is
   * pressed over the control.
   */
  void SetOnMouseDownNotify(OnMouseDownCallback_t OnMouseDownCallback) {
    mOnMouseDownCallback = OnMouseDownCallback;
  }

protected:
  /**
   * The callback function for painting the content of the control
   * @see SetOnPaintNotify()
   */
  OnPaintCallback_t mOnPaintCallback;

  /**
   * The callback function that is called when the mouse is
   * pressed over the control
   * @see SetOnMouseDownNotify()
   */
  OnMouseDownCallback_t mOnMouseDownCallback;

  /** from class PaintWindow */
  virtual void on_paint(Canvas &canvas);

  /** from class Window */
  virtual bool on_mouse_down(PixelScalar x, PixelScalar y);
};

#endif
