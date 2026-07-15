// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The XCSoar Project

#include "MenuBar.hpp"
#include "ui/window/ContainerWindow.hpp"
#include "Input/InputEvents.hpp"

#include <algorithm>
#include <cassert>

[[gnu::pure]]
static PixelRect
GetButtonPosition(unsigned i, PixelRect rc)
{
  const unsigned screen_width = rc.GetWidth();
  const unsigned screen_height = rc.GetHeight();
  const bool portrait = screen_height > screen_width;

  unsigned width = std::max(1u, screen_width / (portrait ? 4u : 5u));
  unsigned height = std::max(1u,
    screen_height / (portrait ? menubar_height_scale_factor : 5u));

  if (i == 0) {
    rc.left = rc.right;
    rc.top = rc.bottom;
  } else if (i < 5) {
    if (portrait) {
      rc.left += width * (i - 1);
      rc.top = rc.bottom - height;
    } else
      rc.top += height * (i - 1);
  } else {
    if (portrait)
      width = std::max(1u, screen_width / 3);

    rc.left = rc.right - width;
    rc.top += (i - 5) * height;
  }

  rc.right = rc.left + width;
  rc.bottom = rc.top + height;
  return rc;
}

bool
MenuBar::Button::OnClicked() noexcept
{
  if (event > 0)
    InputEvents::ProcessEvent(event);
  return true;
}

MenuBar::MenuBar(ContainerWindow &parent, const ButtonLook &_look)
  :look(_look)
{
  const PixelRect rc = parent.GetClientRect();

  WindowStyle style;
  style.Hide();
  style.Border();

  for (unsigned i = 0; i < MAX_BUTTONS; ++i) {
    PixelRect button_rc = GetButtonPosition(i, rc);
    buttons[i].Create(parent, look, "", button_rc, style);
  }
}

void
MenuBar::ShowButton(unsigned i, bool enabled, const char *text,
                    unsigned event)
{
  assert(i < MAX_BUTTONS);

  Button &button = buttons[i];

  button.SetMenuCaption(look, text);
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
