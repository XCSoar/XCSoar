/*
Copyright_License {

  XCSoar Glide Computer - http://www.xcsoar.org/
  Copyright (C) 2000-2015 The XCSoar Project
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
#include "Asset.hpp"
#include "Renderer/TextButtonRenderer.hpp"

void
WndButton::Create(ContainerWindow &parent,
                  const PixelRect &rc,
                  ButtonWindowStyle style,
                  ButtonRenderer *_renderer)
{
  renderer = _renderer;

  style.EnableCustomPainting();
  ButtonWindow::Create(parent, _T(""), rc, style);
}

void
WndButton::Create(ContainerWindow &parent, const ButtonLook &look,
                  tstring::const_pointer caption, const PixelRect &rc,
                  ButtonWindowStyle style)
{
  Create(parent, rc, style, new TextButtonRenderer(look, caption));
}

void
WndButton::Create(ContainerWindow &parent, const PixelRect &rc,
                  ButtonWindowStyle style, ButtonRenderer *_renderer,
                  ActionListener &_listener, int _id)
{
  renderer = _renderer;
  listener = &_listener;

  style.EnableCustomPainting();
#ifdef USE_GDI
  /* use BaseButtonWindow::COMMAND_BOUNCE_ID */
  id = _id;
  ButtonWindow::Create(parent, _T(""), rc, style);
#else
  /* our custom SDL/OpenGL button doesn't need this hack */
  ButtonWindow::Create(parent, _T(""), _id, rc, style);
#endif
}

void
WndButton::Create(ContainerWindow &parent, const ButtonLook &look,
                  tstring::const_pointer caption, const PixelRect &rc,
                  ButtonWindowStyle style,
                  ActionListener &_listener, int _id) {
  renderer = new TextButtonRenderer(look, caption);

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

void
WndButton::SetCaption(tstring::const_pointer caption)
{
  assert(caption != nullptr);

  TextButtonRenderer &r = *(TextButtonRenderer *)renderer;
  r.SetCaption(caption);
}

unsigned
WndButton::GetMinimumWidth() const
{
  return renderer->GetMinimumButtonWidth();
}

bool
WndButton::OnClicked()
{
  if (listener != nullptr) {
#ifndef USE_GDI
    unsigned id = GetID();
#endif
    listener->OnAction(id);
    return true;
  }

  return ButtonWindow::OnClicked();
}

void
WndButton::OnDestroy()
{
  assert(renderer != nullptr);

  delete renderer;

  ButtonWindow::OnDestroy();
}

#ifdef USE_GDI

void
WndButton::OnSetFocus()
{
  ButtonWindow::OnSetFocus();

  /* GDI's "BUTTON" class on Windows CE Core (e.g. Altair) does not
     repaint when the window gets focus, but our custom style requires
     it */
  ::InvalidateRect(hWnd, nullptr, false);
}

void
WndButton::OnKillFocus()
{
  ButtonWindow::OnKillFocus();

  /* GDI's "BUTTON" class does not repaint when the window loses
     focus, but our custom style requires it */
  ::InvalidateRect(hWnd, nullptr, false);
}

#endif

void
WndButton::OnPaint(Canvas &canvas)
{
  assert(renderer != nullptr);

  const bool pressed = IsDown();
  const bool focused = HasCursorKeys() ? HasFocus() : pressed;

  renderer->DrawButton(canvas, GetClientRect(),
                       IsEnabled(), focused, pressed);
}
