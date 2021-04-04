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

#ifndef XCSOAR_OFFSET_BUTTONS_WIDGET_HPP
#define XCSOAR_OFFSET_BUTTONS_WIDGET_HPP

#include "Widget.hpp"
#include "Form/Button.hpp"

#include <array>
#include <memory>

#include <array>

#include <tchar.h>

struct ButtonLook;
class Button;

/**
 * Show four buttons to increment/decrement a value.
 */
class OffsetButtonsWidget : public NullWidget {
  const ButtonLook &look;
  const TCHAR *const format;
  const double offsets[4];
  std::unique_ptr<std::array<Button, 4>> buttons;

public:
  OffsetButtonsWidget(const ButtonLook &_look, const TCHAR *_format,
                      double small_offset, double large_offset) noexcept
    :look(_look), format(_format),
     offsets{-large_offset, -small_offset, small_offset, large_offset} {}

public:
  /* virtual methods from Widget */
  PixelSize GetMinimumSize() const noexcept override;
  PixelSize GetMaximumSize() const noexcept override;
  void Prepare(ContainerWindow &parent, const PixelRect &rc) noexcept override;
  void Show(const PixelRect &rc) noexcept override;
  void Hide() noexcept override;
  void Move(const PixelRect &rc) noexcept override;
  bool SetFocus() noexcept override;

protected:
  virtual void OnOffset(double offset) noexcept = 0;

private:
  Button MakeButton(ContainerWindow &parent, const PixelRect &r,
                    unsigned i) noexcept;
  std::array<Button, 4> MakeButtons(ContainerWindow &parent,
                                    const PixelRect &r) noexcept;
};

#endif
