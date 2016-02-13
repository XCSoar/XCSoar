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

#include "MenuBar.hpp"
#include "Screen/ContainerWindow.hpp"
#include "Input/InputEvents.hpp"

#include <assert.h>

gcc_pure
static PixelRect
GetButtonPosition(unsigned i, PixelRect rc)
{
  unsigned hwidth = rc.GetWidth(), hheight = rc.GetHeight();

  if (hheight > hwidth) {
    // portrait

    hheight /= 6;

    if (i == 0) {
      rc.left = rc.right;
      rc.top = rc.bottom;
    } else if (i < 5) {
      hwidth /= 4;

      rc.left += hwidth * (i - 1);
      rc.top = rc.bottom - hheight;
    } else {
      hwidth /= 3;

      rc.left = rc.right - hwidth;
      rc.top += (i - 5) * hheight;
    }

    rc.right = rc.left + hwidth;
    rc.bottom = rc.top + hheight;
  } else {
    // landscape

    hwidth /= 5;
    hheight /= 5;

    if (i == 0) {
      rc.left = rc.right;
      rc.top = rc.bottom;
    } else if (i < 5) {
      rc.top += hheight * (i - 1);
    } else {
      rc.left += hwidth * (i - 5);
      rc.top = rc.bottom - hheight;
    }

    rc.right = rc.left + hwidth;
    rc.bottom = rc.top + hheight;
  }

  return rc;
}

bool
MenuBar::Button::OnClicked()
{
  if (event > 0)
    InputEvents::ProcessEvent(event);
  return true;
}

MenuBar::MenuBar(ContainerWindow &parent, const ButtonLook &look)
{
  const PixelRect rc = parent.GetClientRect();

  WindowStyle style;
  style.Hide();
  style.Border();

  for (unsigned i = 0; i < MAX_BUTTONS; ++i) {
    PixelRect button_rc = GetButtonPosition(i, rc);
    buttons[i].Create(parent, look, _T(""), button_rc, style);
  }
}

void
MenuBar::ShowButton(unsigned i, bool enabled, const TCHAR *text,
                    unsigned event)
{
  assert(i < MAX_BUTTONS);

  Button &button = buttons[i];

  button.SetCaption(text);
  button.SetEnabled(enabled && event > 0);
  button.SetEvent(event);
  button.ShowOnTop();
}

void
MenuBar::HideButton(unsigned i)
{
  assert(i < MAX_BUTTONS);

  buttons[i].Hide();
}

void
MenuBar::OnResize(const PixelRect &rc)
{
  for (unsigned i = 0; i < MAX_BUTTONS; ++i)
    buttons[i].Move(GetButtonPosition(i, rc));
}
