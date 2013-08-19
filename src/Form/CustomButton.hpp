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

#ifndef XCSOAR_FORM_CUSTOMBUTTON_HPP
#define XCSOAR_FORM_CUSTOMBUTTON_HPP

#include "Form/Button.hpp"

struct DialogLook;

/**
 * This class is used for creating buttons that are custom painted.
 * It is based on the WndButton class.
 */
class WndCustomButton : public WndButton
{
  const DialogLook &look;

public:
  /**
   * Constructor of the WndCustomButton class
   * @param Parent Parent window/ContainerControl
   * @param Name Name of the button
   * @param Caption Text on the button
   */
  WndCustomButton(ContainerWindow &Parent, const DialogLook &_look,
                  tstring::const_pointer Caption,
                  const PixelRect &rc, const ButtonWindowStyle style,
                  ActionListener &listener, int id);

protected:
  /**
   * The OnPaint event is called when the button needs to be drawn
   * (derived from PaintWindow)
   */
  virtual void OnPaint(Canvas &canvas) override;
};

#endif
