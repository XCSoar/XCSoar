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

#ifndef XCSOAR_BUTTON_WIDGET_HPP
#define XCSOAR_BUTTON_WIDGET_HPP

#include "WindowWidget.hpp"

#include <functional>
#include <memory>

#include <tchar.h>

struct ButtonLook;
class Button;
class ButtonRenderer;

/**
 * A #Widget that creates a #Button.
 */
class ButtonWidget : public WindowWidget {
  std::unique_ptr<ButtonRenderer> renderer;
  const std::function<void()> callback;

public:
  ButtonWidget(std::unique_ptr<ButtonRenderer> _renderer,
               std::function<void()> _callback) noexcept;

  ButtonWidget(const ButtonLook &look, const TCHAR *caption,
               std::function<void()> _callback) noexcept;

  ~ButtonWidget() noexcept override;

  ButtonRenderer &GetRenderer() noexcept;
  const ButtonRenderer &GetRenderer() const noexcept;

  /**
   * Schedule a repaint.
   */
  void Invalidate() noexcept;

  /* virtual methods from class Widget */
  PixelSize GetMinimumSize() const noexcept override;
  PixelSize GetMaximumSize() const noexcept override;
  void Initialise(ContainerWindow &parent, const PixelRect &rc) noexcept override;
  bool SetFocus() noexcept override;
};

#endif
