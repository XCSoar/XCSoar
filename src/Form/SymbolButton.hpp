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

#ifndef XCSOAR_FORM_SYMBOLBUTTON_HPP
#define XCSOAR_FORM_SYMBOLBUTTON_HPP

#include "Form/Button.hpp"
#include "Screen/Brush.hpp"
#include "Screen/Features.hpp"

/**
 * This class is used for creating buttons with symbols instead of text.
 * It is based on the WndButton class.
 */
class WndSymbolButton : public WndButton {
  static inline ButtonWindowStyle custom_painting(ButtonWindowStyle style) {
    style.enable_custom_painting();
    return style;
  }

  Brush disabled_brush;
#ifdef HAVE_CLIPPING
  Brush background_brush;
#endif

public:
  /**
   * Constructor of the WndSymbolButton class
   * @param Parent Parent window/ContainerControl
   * @param Name Name of the button
   * @param Caption Text on the button
   * @param X x-Coordinate relative to the parent
   * @param Y y-Coordinate relative to the parent
   * @param Width Width of the button
   * @param Height Height of the button
   * @param Function The function that should be called
   * when the button is clicked
   */
  WndSymbolButton(ContainerWindow &Parent, const TCHAR *Caption,
      int X, int Y, int Width, int Height, const ButtonWindowStyle style,
                  Color background_color,
                  ClickNotifyCallback_t Function = NULL)
    :WndButton(Parent, Caption, X, Y, Width, Height,
               custom_painting(style), Function),
               disabled_brush(COLOR_GRAY)
#ifdef HAVE_CLIPPING
    , background_brush(background_color)
#endif
  {}

protected:
  /**
   * The on_paint event is called when the button needs to be drawn
   * (derived from PaintWindow)
   */
  virtual void on_paint(Canvas &canvas);
};

#endif
