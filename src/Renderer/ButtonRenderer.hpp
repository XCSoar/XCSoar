/*
Copyright_License {

  XCSoar Glide Computer - http://www.xcsoar.org/
  Copyright (C) 2000-2021 The XCSoar Project
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

#ifndef XCSOAR_BUTTON_RENDERER_HPP
#define XCSOAR_BUTTON_RENDERER_HPP

struct PixelRect;
struct ButtonLook;
class Canvas;

enum class ButtonState : int {
  /**
   * The button is disabled, i.e. inaccessible.
   */
  DISABLED,

  /**
   * The button is enabled (but not focused).
   */
  ENABLED,

  /**
   * The button is selected, but is not the currently focused control.
   */
  SELECTED,

  /**
   * The button is the currently focused control.
   */
  FOCUSED,

  /**
   * The button is currently pressed down.
   */
  PRESSED,
};

class ButtonFrameRenderer {
  const ButtonLook &look;

public:
  explicit ButtonFrameRenderer(const ButtonLook &_look) noexcept:look(_look) {}

  const ButtonLook &GetLook() const noexcept {
    return look;
  }

  [[gnu::const]]
  static unsigned GetMargin() noexcept;

  void DrawButton(Canvas &canvas, PixelRect rc,
                  ButtonState state) const noexcept;

  [[gnu::pure]]
  PixelRect GetDrawingRect(PixelRect rc, ButtonState state) const noexcept;
};

class ButtonRenderer {
public:
  virtual ~ButtonRenderer() noexcept = default;

  [[gnu::pure]]
  virtual unsigned GetMinimumButtonWidth() const noexcept;

  virtual void DrawButton(Canvas &canvas, const PixelRect &rc,
                          ButtonState state) const noexcept = 0;
};

#endif
