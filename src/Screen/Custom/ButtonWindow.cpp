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

#include "Screen/ButtonWindow.hpp"
#include "Screen/ContainerWindow.hpp"
#include "Screen/Layout.hpp"
#include "Screen/Key.h"
#include "Hardware/Vibrator.hpp"

void
ButtonWindow::Create(ContainerWindow &parent, const TCHAR *text, unsigned id,
                     const PixelRect &rc,
                     const ButtonWindowStyle style)
{
  Destroy();

  PaintWindow::Create(parent, rc, style);

  this->text = text;
  this->id = id;
}

void
ButtonWindow::set_down(bool _down)
{
  if (_down == down)
    return;

#ifdef HAVE_VIBRATOR
  VibrateShort();
#endif

  down = _down;
  Invalidate();
}

bool
ButtonWindow::OnKeyCheck(unsigned key_code) const
{
  switch (key_code) {
  case KEY_RETURN:
    return true;

  default:
    return PaintWindow::OnKeyCheck(key_code);
  }
}

bool
ButtonWindow::OnKeyDown(unsigned key_code)
{
  switch (key_code) {
  case KEY_RETURN:
  case KEY_SPACE:
    set_down(false);

    if (!OnClicked() && id != 0 && parent != NULL)
      parent->OnCommand(id, 0);
    return true;

  default:
    return PaintWindow::OnKeyDown(key_code);
  }
}

bool
ButtonWindow::OnMouseMove(PixelScalar x, PixelScalar y, unsigned keys)
{
  if (dragging) {
    set_down(x >= 0 && y >= 0 &&
             (unsigned)x < GetWidth() && (unsigned)y < GetHeight());
    return true;
  } else
    return PaintWindow::OnMouseMove(x, y, keys);
}

bool
ButtonWindow::OnMouseDown(PixelScalar x, PixelScalar y)
{
  if (IsTabStop())
    SetFocus();

  set_down(true);
  SetCapture();
  dragging = true;
  return true;
}

bool
ButtonWindow::OnMouseUp(PixelScalar x, PixelScalar y)
{
  if (!dragging)
    return true;

  dragging = false;
  ReleaseCapture();

  if (!down)
    return true;

  set_down(false);

  if (!OnClicked() && id != 0 && parent != NULL)
    parent->OnCommand(id, 0);

  return true;
}

void
ButtonWindow::OnSetFocus()
{
  PaintWindow::OnSetFocus();
  Invalidate();
}

void
ButtonWindow::OnKillFocus()
{
  PaintWindow::OnKillFocus();
  Invalidate();
}

bool
ButtonWindow::OnCancelMode()
{
  dragging = false;
  set_down(false);

  return PaintWindow::OnCancelMode();
}

void
ButtonWindow::OnPaint(Canvas &canvas)
{
  if (HasFocus()) {
    Pen pen(Layout::Scale(1), COLOR_BLACK);
    canvas.Select(pen);
    canvas.SelectHollowBrush();
    canvas.Rectangle(-1, -1, canvas.GetWidth(), canvas.GetHeight());
  }

  PixelRect rc = {
    2, 2, PixelScalar(canvas.GetWidth() - 4),
    PixelScalar(canvas.GetHeight() - 4),
  };

  if (down) {
    rc.left += Layout::FastScale(1);
    rc.top += Layout::FastScale(1);
  }

  canvas.DrawButton(GetClientRect(), down);

  canvas.SetTextColor(IsEnabled() ? COLOR_BLACK : COLOR_GRAY);
  canvas.SetBackgroundTransparent();
  canvas.formatted_text(&rc, text.c_str(), GetTextStyle());
}

bool
ButtonWindow::OnClicked()
{
  return false;
}
