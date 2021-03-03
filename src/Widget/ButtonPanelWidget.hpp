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

#ifndef XCSOAR_BUTTON_PANEL_WIDGET_HPP
#define XCSOAR_BUTTON_PANEL_WIDGET_HPP

#include "Widget.hpp"

#include <cassert>
#include <memory>

class ButtonPanel;

/**
 * Wrap a #ButtonPanel around another #Widget instance.
 */
class ButtonPanelWidget : public Widget {
public:
  enum class Alignment {
    AUTO, BOTTOM, LEFT
  };

private:
  const std::unique_ptr<Widget> widget;
  std::unique_ptr<ButtonPanel> buttons;
  Alignment alignment;

public:
  ButtonPanelWidget(std::unique_ptr<Widget> &&_widget,
                    Alignment _alignment=Alignment::AUTO) noexcept
    :widget(std::move(_widget)), alignment(_alignment) {}

  ~ButtonPanelWidget() noexcept override;

  Widget &GetWidget() noexcept {
    return *widget;
  }

  ButtonPanel &GetButtonPanel() noexcept {
    assert(buttons != nullptr);

    return *buttons;
  }

private:
  PixelRect UpdateLayout(const PixelRect &rc) noexcept;

public:
  PixelSize GetMinimumSize() const noexcept override;
  PixelSize GetMaximumSize() const noexcept override;
  void Initialise(ContainerWindow &parent, const PixelRect &rc) noexcept override;
  void Prepare(ContainerWindow &parent, const PixelRect &rc) noexcept override;
  void Unprepare() noexcept override;
  bool Save(bool &changed) noexcept override;
  bool Click() noexcept override;
  void ReClick() noexcept override;
  void Show(const PixelRect &rc) noexcept override;
  bool Leave() noexcept override;
  void Hide() noexcept override;
  void Move(const PixelRect &rc) noexcept override;
  bool SetFocus() noexcept override;
  bool KeyPress(unsigned key_code) noexcept override;
};

#endif
