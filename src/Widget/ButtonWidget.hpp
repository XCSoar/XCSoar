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

#ifndef XCSOAR_BUTTON_WIDGET_HPP
#define XCSOAR_BUTTON_WIDGET_HPP

#include "WindowWidget.hpp"

#include <tchar.h>

struct ButtonLook;
class Button;
class ButtonRenderer;
class ActionListener;

/**
 * A #Widget that creates a #Button.
 */
class ButtonWidget : public WindowWidget {
  ButtonRenderer *const renderer;
  ActionListener &listener;
  int id;

public:
  ButtonWidget(ButtonRenderer *_renderer, ActionListener &_listener, int _id)
    :renderer(_renderer), listener(_listener), id(_id) {}

  ButtonWidget(const ButtonLook &look, const TCHAR *caption,
               ActionListener &_listener, int _id);

  virtual ~ButtonWidget();

  ButtonRenderer &GetRenderer() {
    return *renderer;
  }

  /**
   * Schedule a repaint.
   */
  void Invalidate();

  /* virtual methods from class Widget */
  PixelSize GetMinimumSize() const override;
  PixelSize GetMaximumSize() const override;
  void Initialise(ContainerWindow &parent, const PixelRect &rc) override;
  bool SetFocus() override;
};

#endif
