// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The XCSoar Project

#include "MenuBar.hpp"
#include "ui/window/ContainerWindow.hpp"
#include "Input/InputEvents.hpp"

#include <cassert>

[[gnu::pure]]
static PixelRect
GetButtonPosition(unsigned i, PixelRect rc)
{
  unsigned hwidth = rc.GetWidth(), hheight = rc.GetHeight();

  if (hheight > hwidth) {
    // portrait

    hheight /= menubar_height_scale_factor;

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
MenuBar::Button::OnClicked() noexcept
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
MenuBar::ShowButton(unsigned i, bool enabled, const char *text,
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
