/*
Copyright_License {

  XCSoar Glide Computer - http://www.xcsoar.org/
  Copyright (C) 2000-2013 The XCSoar Project
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

#include "Form/Button.hpp"
#include "Form/ActionListener.hpp"
#include "Look/ButtonLook.hpp"
#include "Screen/Key.h"
#include "Screen/Canvas.hpp"
#include "Asset.hpp"

WndButton::WndButton(ContainerWindow &parent, const ButtonLook &look,
                     const TCHAR *Caption, const PixelRect &rc,
                     ButtonWindowStyle style,
                     ClickNotifyCallback _click_callback)
  :renderer(look),
   listener(NULL),
   click_callback(nullptr)
{
  Create(parent, Caption, rc, style, _click_callback);
}

WndButton::WndButton(ContainerWindow &parent, const ButtonLook &look,
                     const TCHAR *caption, const PixelRect &rc,
                     ButtonWindowStyle style,
                     ActionListener &_listener, int _id)
  :renderer(look), listener(nullptr), click_callback(nullptr)
{
  Create(parent, caption, rc, style, _listener, _id);
}

WndButton::WndButton(const ButtonLook &_look)
  :renderer(_look), listener(nullptr), click_callback(nullptr) {}

void
WndButton::Create(ContainerWindow &parent,
                  tstring::const_pointer caption, const PixelRect &rc,
                  ButtonWindowStyle style,
                  ClickNotifyCallback _click_callback)
{
  click_callback = _click_callback;

  style.EnableCustomPainting();
  ButtonWindow::Create(parent, caption, rc, style);
}

void
WndButton::Create(ContainerWindow &parent,
                  tstring::const_pointer caption, const PixelRect &rc,
                  ButtonWindowStyle style,
                  ActionListener &_listener, int _id) {
  assert(click_callback == nullptr);

  listener = &_listener;

  style.EnableCustomPainting();
#ifdef USE_GDI
  /* use BaseButtonWindow::COMMAND_BOUNCE_ID */
  id = _id;
  ButtonWindow::Create(parent, caption, rc, style);
#else
  /* our custom SDL/OpenGL button doesn't need this hack */
  ButtonWindow::Create(parent, caption, _id, rc, style);
#endif
}

bool
WndButton::OnClicked()
{
  if (listener != NULL) {
#ifndef USE_GDI
    unsigned id = GetID();
#endif
    listener->OnAction(id);
    return true;
  }

  // Call the OnClick function
  if (click_callback != NULL) {
    click_callback();
    return true;
  }

  return ButtonWindow::OnClicked();
}

#ifdef USE_GDI

void
WndButton::OnSetFocus()
{
  ButtonWindow::OnSetFocus();

  /* GDI's "BUTTON" class on Windows CE Core (e.g. Altair) does not
     repaint when the window gets focus, but our custom style requires
     it */
  ::InvalidateRect(hWnd, NULL, false);
}

void
WndButton::OnKillFocus()
{
  ButtonWindow::OnKillFocus();

  /* GDI's "BUTTON" class does not repaint when the window loses
     focus, but our custom style requires it */
  ::InvalidateRect(hWnd, NULL, false);
}

#endif

void
WndButton::OnPaint(Canvas &canvas)
{
  const ButtonLook &look = renderer.GetLook();

  const bool pressed = IsDown();
  const bool focused = HasCursorKeys() ? HasFocus() : pressed;

  PixelRect rc = canvas.GetRect();
  renderer.DrawButton(canvas, rc, focused, pressed);

  // If button has text on it
  const tstring caption = GetText();
  if (caption.empty())
    return;

  rc = renderer.GetDrawingRect(rc, pressed);

  canvas.SetBackgroundTransparent();
  if (!IsEnabled())
    canvas.SetTextColor(look.disabled.color);
  else if (focused)
    canvas.SetTextColor(look.focused.foreground_color);
  else
    canvas.SetTextColor(look.standard.foreground_color);

  canvas.Select(*look.font);

#ifndef USE_GDI
  unsigned style = GetTextStyle();

  if (IsDithered())
    style |= DT_UNDERLINE;

  canvas.DrawFormattedText(&rc, caption.c_str(), style);
#else
  unsigned style = DT_CENTER | DT_NOCLIP | DT_WORDBREAK;

  PixelRect text_rc = rc;
  canvas.DrawFormattedText(&text_rc, caption.c_str(), style | DT_CALCRECT);
  text_rc.right = rc.right;

  PixelScalar offset = rc.bottom - text_rc.bottom;
  if (offset > 0) {
    offset /= 2;
    text_rc.top += offset;
    text_rc.bottom += offset;
  }

  canvas.DrawFormattedText(&text_rc, caption.c_str(), style);
#endif
}
