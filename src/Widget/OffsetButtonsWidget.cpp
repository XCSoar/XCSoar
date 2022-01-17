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

#include "OffsetButtonsWidget.hpp"
#include "Screen/Layout.hpp"

#include <algorithm>  // for std::any_of
#include <stdio.h>

PixelSize
OffsetButtonsWidget::GetMinimumSize() const noexcept
{
  return { 4u * Layout::GetMinimumControlHeight(),
      Layout::GetMinimumControlHeight() };
}

PixelSize
OffsetButtonsWidget::GetMaximumSize() const noexcept
{
  return { 4u * Layout::GetMaximumControlHeight(),
      Layout::GetMaximumControlHeight() };
}

static constexpr std::array<PixelRect, 4>
LayoutOffsetButtons(const PixelRect &total_rc) noexcept
{
  const unsigned total_width = total_rc.GetWidth();
  PixelRect rc = { 0, total_rc.top, total_rc.left, total_rc.bottom };

  std::array<PixelRect, 4> buttons{};
  for (unsigned i = 0; i < 4; ++i) {
    rc.left = rc.right;
    rc.right = total_rc.left + (i + 1) * total_width / 4;
    buttons[i] = rc;
  }

  return buttons;
}

inline Button
OffsetButtonsWidget::MakeButton(ContainerWindow &parent, const PixelRect &r,
                                unsigned i) noexcept
{
  TCHAR caption[16];
  _stprintf(caption, format, offsets[i]);

  WindowStyle style;
  style.TabStop();
  style.Hide();

  return Button(parent, look, caption, r, style, [this, i](){
    OnOffset(offsets[i]);
  });
}

inline std::array<Button, 4>
OffsetButtonsWidget::MakeButtons(ContainerWindow &parent,
                                 const PixelRect &total) noexcept
{
  const auto r = LayoutOffsetButtons(total);

  return {
    MakeButton(parent, r[0], 0),
    MakeButton(parent, r[1], 1),
    MakeButton(parent, r[2], 2),
    MakeButton(parent, r[3], 3),
  };
}

void
OffsetButtonsWidget::Prepare(ContainerWindow &parent,
                             const PixelRect &total_rc) noexcept
{
  buttons.reset(new std::array<Button, 4>{MakeButtons(parent, total_rc)});
}

void
OffsetButtonsWidget::Show(const PixelRect &total_rc) noexcept
{
  const auto rc = LayoutOffsetButtons(total_rc);

  for (unsigned i = 0; i < buttons->size(); ++i)
    (*buttons)[i].MoveAndShow(rc[i]);
}

void
OffsetButtonsWidget::Hide() noexcept
{
  for (auto &i : *buttons)
    i.Hide();
}

void
OffsetButtonsWidget::Move(const PixelRect &total_rc) noexcept
{
  const auto rc = LayoutOffsetButtons(total_rc);

  for (unsigned i = 0; i < buttons->size(); ++i)
    (*buttons)[i].Move(rc[i]);
}

bool
OffsetButtonsWidget::SetFocus() noexcept
{
  (*buttons)[2].SetFocus();
  return true;
}

bool
OffsetButtonsWidget::HasFocus() const noexcept
{
  return std::any_of(buttons->begin(), buttons->end(), [](const Button &b){
    return b.HasFocus();
  });
}
