/*
Copyright_License {

  XCSoar Glide Computer - http://www.xcsoar.org/
  Copyright (C) 2000-2022 The XCSoar Project
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

#pragma once

#include "WindowWidget.hpp"
#include "Form/VScrollPanel.hpp"

struct DialogLook;

/**
 * A #Widget implementation which shows a vertical scroll bar if the
 * hosted #Widget is larger than the available space.
 */
class VScrollWidget final : public WindowWidget, VScrollPanelListener {
  const DialogLook &look;

  std::unique_ptr<Widget> widget;

  bool visible = false;

public:
  explicit VScrollWidget(std::unique_ptr<Widget> &&_widget,
                         const DialogLook &_look) noexcept
    :look(_look),
     widget(std::move(_widget)) {}

  Widget &GetWidget() noexcept {
    return *widget;
  }

  /* virtual methods from class Widget */
  PixelSize GetMinimumSize() const noexcept override;
  PixelSize GetMaximumSize() const noexcept override;
  void Initialise(ContainerWindow &parent, const PixelRect &rc) noexcept override;
  void Prepare(ContainerWindow &parent, const PixelRect &rc) noexcept override;
  bool Save(bool &changed) noexcept override;
  bool Click() noexcept override;
  void ReClick() noexcept override;
  void Show(const PixelRect &rc) noexcept override;
  bool Leave() noexcept override;
  void Hide() noexcept override;
  bool SetFocus() noexcept override;
  bool HasFocus() const noexcept override;
  bool KeyPress(unsigned key_code) noexcept override;

private:
  VScrollPanel &GetWindow() noexcept {
    return (VScrollPanel &)WindowWidget::GetWindow();
  }

  [[gnu::pure]]
  unsigned CalcVirtualHeight(const PixelRect &rc) const noexcept;

  void UpdateVirtualHeight(const PixelRect &rc) noexcept;

  /* virtual methods from class VScrollPanelListener */
  void OnVScrollPanelChange() noexcept override;
};
