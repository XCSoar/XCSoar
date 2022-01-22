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

#ifndef XCSOAR_ARROW_PAGER_WIDGET_HPP
#define XCSOAR_ARROW_PAGER_WIDGET_HPP

#include "PagerWidget.hpp"
#include "Form/Button.hpp"

#include <cassert>
#include <functional>
#include <memory>

struct ButtonLook;

/**
 * A wrapper for #PagerWidget that adds arrow buttons on the
 * left/bottom for page navigation.
 */
class ArrowPagerWidget : public PagerWidget {
  enum Buttons {
    PREVIOUS,
    NEXT,
  };

  struct Layout {
    PixelRect previous_button, next_button;
    PixelRect close_button;
    PixelRect main;
    PixelRect extra;

    Layout(const ButtonLook &look, PixelRect rc,
           const Widget *extra) noexcept;
  };

  const ButtonLook &look;
  const std::function<void()> close_callback;

  /**
   * An optional #Widget that is shown in the remaining area in the
   * buttons row/column.  This object will be deleted automatically.
   */
  const std::unique_ptr<Widget> extra;

  Button previous_button, next_button;
  Button close_button;

public:
  ArrowPagerWidget(const ButtonLook &_look,
                   std::function<void()> _close_callback,
                   std::unique_ptr<Widget> _extra=nullptr) noexcept
    :look(_look),
     close_callback(std::move(_close_callback)),
     extra(std::move(_extra)) {}

  Widget &GetExtra() noexcept {
    assert(extra != nullptr);

    return *extra;
  }

  /* virtual methods from Widget */
  PixelSize GetMinimumSize() const noexcept override;
  PixelSize GetMaximumSize() const noexcept override;
  void Initialise(ContainerWindow &parent, const PixelRect &rc) noexcept override;
  void Prepare(ContainerWindow &parent, const PixelRect &rc) noexcept override;
  void Show(const PixelRect &rc) noexcept override;
  void Hide() noexcept override;
  void Move(const PixelRect &rc) noexcept override;
  bool SetFocus() noexcept override;
  bool HasFocus() const noexcept override;
  bool KeyPress(unsigned key_code) noexcept override;
};

#endif
