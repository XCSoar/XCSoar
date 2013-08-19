/*
Copyright_License {

  XCSoar Glide Computer - http://www.xcsoar.org/
  Copyright (C) 2000-2013 The XCSoar Project
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

/**
 * This class is used for creating buttons with symbols instead of text.
 * It is based on the WndButton class.
 */
class WndSymbolButton final : public WndButton
{
public:
  /**
   * Constructor of the WndSymbolButton class
   * @param Parent Parent window/ContainerControl
   * @param Name Name of the button
   * @param Caption Text on the button
   * @param Function The function that should be called
   * when the button is clicked
   */
  WndSymbolButton(ContainerWindow &Parent, const ButtonLook &look,
                  tstring::const_pointer Caption,
                  const PixelRect &rc, const ButtonWindowStyle style,
                  ClickNotifyCallback Function = NULL)
    :WndButton(Parent, look, Caption, rc,
               style, Function) {}

  WndSymbolButton(ContainerWindow &Parent, const ButtonLook &look,
                  tstring::const_pointer Caption,
                  const PixelRect &rc, const ButtonWindowStyle style,
                  ActionListener &listener, int id)
    :WndButton(Parent, look, Caption, rc,
               style, listener, id) {}

  WndSymbolButton(const ButtonLook &look)
    :WndButton(look) {}

  using WndButton::Create;

protected:
  virtual void OnPaint(Canvas &canvas) override;
};

#endif
