/*
Copyright_License {

  XCSoar Glide Computer - http://www.xcsoar.org/
  Copyright (C) 2000-2012 The XCSoar Project
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

#include "Form/ButtonPanel.hpp"
#include "Look/DialogLook.hpp"
#include "Screen/ContainerWindow.hpp"
#include "Screen/Layout.hpp"
#include "Screen/Font.hpp"

ButtonPanel::ButtonPanel(ContainerWindow &_parent, const DialogLook &_look)
  :parent(_parent), look(_look) {
  style.TabStop();
}

ButtonPanel::~ButtonPanel()
{
  for (const auto i : buttons)
    delete i;
}

PixelRect
ButtonPanel::UpdateLayout(const PixelRect rc)
{
  if (buttons.empty())
    return rc;

  const bool landscape = rc.right - rc.left > rc.bottom - rc.top;
  return landscape
    ? LeftLayout(rc)
    : BottomLayout(rc);
}

PixelRect
ButtonPanel::UpdateLayout()
{
  return UpdateLayout(parent.GetClientRect());
}

static constexpr PixelRect dummy_rc = { 0, 0, 100, 40 };

WndButton *
ButtonPanel::Add(const TCHAR *caption, ActionListener &listener, int id)
{
  WndButton *button = new WndButton(parent, look, caption,
                                    dummy_rc, style, listener, id);
  buttons.append(button);

  return button;
}

UPixelScalar
ButtonPanel::Width(unsigned i) const
{
  return look.button.font->TextSize(buttons[i]->GetText().c_str()).cx +
    Layout::SmallScale(8);
}

UPixelScalar
ButtonPanel::RangeMaxWidth(unsigned start, unsigned end) const
{
  UPixelScalar max_width = Layout::Scale(50);
  for (unsigned i = start; i < end; ++i) {
    UPixelScalar width = Width(i);
    if (width > max_width)
      max_width = width;
  }

  return max_width;
}

PixelRect
ButtonPanel::VerticalRange(PixelRect rc, unsigned start, unsigned end)
{
  const unsigned n = end - start;
  assert(n > 0);

  const UPixelScalar width = RangeMaxWidth(start, end);
  const UPixelScalar total_height = rc.bottom - rc.top;
  const UPixelScalar max_height = n * Layout::GetMaximumControlHeight();
  const UPixelScalar row_height = std::min(total_height, max_height) / n;

  PixelRect button_rc(rc.left, rc.top, rc.left + width, rc.top + row_height);
  rc.left += width;

  for (unsigned i = start; i < end; ++i) {
    buttons[i]->Move(button_rc);

    button_rc.top = button_rc.bottom;
    button_rc.bottom += row_height;
  }

  return rc;
}

PixelRect
ButtonPanel::HorizontalRange(PixelRect rc, unsigned start, unsigned end)
{
  const unsigned n = end - start;
  assert(n > 0);

  const UPixelScalar total_width = rc.right - rc.left;
  const UPixelScalar row_height = Layout::GetMaximumControlHeight();
  const UPixelScalar width = total_width / n;
  assert(width > 0);

  PixelRect button_rc(rc.left, rc.bottom - row_height,
                      rc.left + width, rc.bottom);
  rc.bottom -= row_height;

  for (unsigned i = start; i < end; ++i) {
    buttons[i]->Move(button_rc);

    button_rc.left = button_rc.right;
    button_rc.right += width;
  }

  return rc;
}

PixelRect
ButtonPanel::LeftLayout(PixelRect rc)
{
  assert(!buttons.empty());

  return VerticalRange(rc, 0, buttons.size());
}

PixelRect
ButtonPanel::LeftLayout()
{
  return LeftLayout(parent.GetClientRect());
}

PixelRect
ButtonPanel::BottomLayout(PixelRect rc)
{
  assert(!buttons.empty());

  const UPixelScalar total_width = rc.right - rc.left;

  unsigned end = buttons.size();
  while (end > 0) {
    unsigned start = end - 1;
    UPixelScalar max_width = Width(start);
    while (start > 0) {
      --start;
      UPixelScalar width = Width(start);
      UPixelScalar new_width = std::max(width, max_width);
      if ((end - start) * new_width > total_width) {
        ++start;
        break;
      }

      max_width = new_width;
    }

    rc = HorizontalRange(rc, start, end);
    end = start;
  }

  return rc;
}

PixelRect
ButtonPanel::BottomLayout()
{
  return BottomLayout(parent.GetClientRect());
}

void
ButtonPanel::ShowAll()
{
  for (auto i : buttons)
    i->Show();
}

void
ButtonPanel::HideAll()
{
  for (auto i : buttons)
    i->Hide();
}
