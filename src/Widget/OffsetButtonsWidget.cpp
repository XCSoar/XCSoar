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

#include "OffsetButtonsWidget.hpp"
#include "Form/Button.hpp"
#include "Util/Macros.hpp"
#include "Screen/Layout.hpp"

#include <stdio.h>

PixelSize
OffsetButtonsWidget::GetMinimumSize() const
{
  return { 4u * Layout::GetMinimumControlHeight(),
      Layout::GetMinimumControlHeight() };
}

PixelSize
OffsetButtonsWidget::GetMaximumSize() const
{
  return { 4u * Layout::GetMaximumControlHeight(),
      Layout::GetMaximumControlHeight() };
}

static void
LayoutOffsetButtons(const PixelRect &total_rc, PixelRect buttons[4])
{
  const unsigned total_width = total_rc.GetWidth();
  PixelRect rc = { 0, total_rc.top, total_rc.left, total_rc.bottom };

  for (unsigned i = 0; i < 4; ++i) {
    rc.left = rc.right;
    rc.right = total_rc.left + (i + 1) * total_width / 4;
    buttons[i] = rc;
  }
}

void
OffsetButtonsWidget::Prepare(ContainerWindow &parent,
                             const PixelRect &total_rc)
{
  PixelRect rc[ARRAY_SIZE(buttons)];
  LayoutOffsetButtons(total_rc, rc);

  WindowStyle style;
  style.TabStop();
  style.Hide();

  for (unsigned i = 0; i < ARRAY_SIZE(buttons); ++i) {
    TCHAR caption[16];
    _stprintf(caption, format, (double)offsets[i]);
    buttons[i] = new Button(parent, look, caption, rc[i], style,
                            *this, i);
  }
}

void
OffsetButtonsWidget::Unprepare()
{
  for (unsigned i = 0; i < ARRAY_SIZE(offsets); ++i)
    delete buttons[i];
}

void
OffsetButtonsWidget::Show(const PixelRect &total_rc)
{
  PixelRect rc[ARRAY_SIZE(buttons)];
  LayoutOffsetButtons(total_rc, rc);

  for (unsigned i = 0; i < ARRAY_SIZE(buttons); ++i)
    buttons[i]->MoveAndShow(rc[i]);
}

void
OffsetButtonsWidget::Hide()
{
  for (unsigned i = 0; i < ARRAY_SIZE(buttons); ++i)
    buttons[i]->Hide();
}

void
OffsetButtonsWidget::Move(const PixelRect &total_rc)
{
  PixelRect rc[ARRAY_SIZE(buttons)];
  LayoutOffsetButtons(total_rc, rc);

  for (unsigned i = 0; i < ARRAY_SIZE(buttons); ++i)
    buttons[i]->Move(rc[i]);
}

bool
OffsetButtonsWidget::SetFocus()
{
  buttons[2]->SetFocus();
  return true;
}

void
OffsetButtonsWidget::OnAction(int id)
{
  assert(unsigned(id) < ARRAY_SIZE(offsets));

  OnOffset(offsets[id]);
}
