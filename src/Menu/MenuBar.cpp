// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The XCSoar Project

#include "MenuBar.hpp"
#include "ui/window/ContainerWindow.hpp"
#include "Renderer/ButtonRenderer.hpp"
#include "Screen/Layout.hpp"
#include "Input/InputEvents.hpp"

#include <algorithm>
#include <cassert>

unsigned
MenuBar::GetButtonHeight(unsigned screen_height, bool portrait) noexcept
{
  unsigned height = std::max(1u,
    screen_height / (portrait ? menubar_height_scale_factor : 5u));
  const unsigned cap = Layout::GetInflightButtonHeight();
  if (portrait && cap > 0 && height > cap)
    height = cap;
  return height;
}

/**
 * The space around a menu button face, the same between neighbours
 * as towards the edge; #ButtonFrameRenderer already insets the face
 * by its margin on every side.
 */
[[gnu::pure]]
static unsigned
GetFaceSpacing() noexcept
{
  return std::max(2 * ButtonFrameRenderer::GetMargin(), Layout::VptScale(6));
}

[[gnu::pure]]
static PixelRect
GetButtonPosition(unsigned i, PixelRect rc)
{
  const bool portrait = rc.GetHeight() > rc.GetWidth();

  if (i == 0) {
    /* the first slot is never shown; park it outside the bottom right
       corner.  It keeps the size of a menu button because
       Window::Move() rejects an empty rectangle */
    const unsigned width = std::max(1u, rc.GetWidth()
                                    / (portrait ? 4u : 5u));
    const unsigned height = MenuBar::GetButtonHeight(rc.GetHeight(),
                                                     portrait);

    rc.left = rc.right;
    rc.top = rc.bottom;
    rc.right = rc.left + int(width);
    rc.bottom = rc.top + int(height);
    return rc;
  }

  const int margin = ButtonFrameRenderer::GetMargin();
  const int spacing = GetFaceSpacing();

  /* what is left of the space once the button's own margin is
     subtracted: two neighbouring slots contribute one inset each,
     the screen edge contributes the outer inset alone */
  const int inner = (spacing - 2 * margin) / 2;
  const int outer = spacing - margin - inner;

  /* on a screen this small the inset would turn the rectangle inside
     out, and GetWidth() would wrap around */
  if (int(rc.GetWidth()) > 2 * outer && int(rc.GetHeight()) > 2 * outer)
    rc.Grow(-outer);

  const unsigned screen_width = rc.GetWidth();
  const unsigned screen_height = rc.GetHeight();

  if (i < 5) {
    /* the four mode buttons: a row along the bottom edge in portrait,
       a column along the left edge in landscape (buttonmenu.png).
       Split the full extent instead of stepping by a rounded-down
       width, so all four come out the same size */
    if (portrait) {
      const int left = rc.left;
      rc.left = left + int(screen_width * (i - 1) / 4);
      rc.right = left + int(screen_width * i / 4);
      rc.top = rc.bottom - int(MenuBar::GetButtonHeight(screen_height,
                                                        portrait));
    } else {
      const int top = rc.top;
      rc.top = top + int(screen_height * (i - 1) / 4);
      rc.bottom = top + int(screen_height * i / 4);
      rc.right = rc.left + int(std::max(1u, screen_width / 5));
    }
  } else {
    /* the menu entries: a column along the right edge, in portrait on
       the same pitch as the mode button row it ends above */
    const unsigned height = MenuBar::GetButtonHeight(screen_height,
                                                     portrait);
    const int top = rc.top;

    rc.left = rc.right - int(std::max(1u, screen_width
                                      / (portrait ? 3u : 5u)));
    rc.top = top + int((i - 5) * height);
    rc.bottom = top + int((i - 4) * height);
  }

  rc.Grow(-inner);

  /* Window::Move() rejects an empty rectangle */
  if (rc.right <= rc.left)
    rc.right = rc.left + 1;
  if (rc.bottom <= rc.top)
    rc.bottom = rc.top + 1;

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

  for (unsigned i = 0; i < MAX_BUTTONS; ++i) {
    PixelRect button_rc = GetButtonPosition(i, rc);
    buttons[i].Create(parent, look, "", button_rc, style);
#ifndef USE_WINUSER
    /* Let the map show through rounded corners / translucent fill. */
    buttons[i].SetTransparent();
#endif
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
