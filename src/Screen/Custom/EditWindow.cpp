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

#include "Screen/EditWindow.hpp"
#include "Screen/Canvas.hpp"
#include "Screen/Features.hpp"
#include "Screen/Layout.hpp"
#include "Screen/Key.h"

void
EditWindow::Create(ContainerWindow &parent, PixelRect rc,
                   const EditWindowStyle style)
{
  read_only = style.is_read_only;
  origin = 0;

  Window::Create(&parent, rc, style);
}

void
EditWindow::ScrollVertically(int delta_lines)
{
  AssertNoneLocked();
  assert(IsMultiLine());

  const unsigned visible_rows = GetVisibleRows();
  const unsigned row_count = GetRowCount();

  if (visible_rows >= row_count)
    /* all rows are visible at a time, no scrolling needed/possible */
    return;

  unsigned new_origin = origin + delta_lines;
  if ((int)new_origin < 0)
    new_origin = 0;
  else if (new_origin > row_count - visible_rows)
    new_origin = row_count - visible_rows;

  if (new_origin != origin) {
    origin = new_origin;
    Invalidate();
  }
}

void
EditWindow::OnResize(UPixelScalar width, UPixelScalar height)
{
  Window::OnResize(width, height);

  if (IsMultiLine() && !value.empty()) {
    /* revalidate the scroll position */
    const unsigned visible_rows = GetVisibleRows();
    const unsigned row_count = GetRowCount();
    if (visible_rows >= row_count)
      origin = 0;
    else if (origin > row_count - visible_rows)
      origin = row_count - visible_rows;
    Invalidate();
  }
}

void
EditWindow::OnPaint(Canvas &canvas)
{
  if (IsEnabled()) {
    if (IsReadOnly())
      canvas.Clear(Color(0xf0, 0xf0, 0xf0));
    else
      canvas.ClearWhite();
    canvas.SetTextColor(COLOR_BLACK);
  } else {
    canvas.Clear(COLOR_LIGHT_GRAY);
    canvas.SetTextColor(COLOR_DARK_GRAY);
  }

  PixelRect rc = {
    0, 0, PixelScalar(canvas.GetWidth() - 1),
    PixelScalar(canvas.GetHeight() - 1),
  };

  canvas.DrawOutlineRectangle(rc.left, rc.top, rc.right, rc.bottom, COLOR_BLACK);

  if (value.empty())
    return;

  canvas.SetBackgroundTransparent();

  const PixelScalar padding = Layout::GetTextPadding();
  GrowRect(rc, -padding, -padding);

  if (HaveClipping() || IsMultiLine()) {
    rc.top -= origin * GetFont().GetHeight();
    canvas.formatted_text(&rc, value.c_str(), GetTextStyle());
  } else if ((GetTextStyle() & DT_VCENTER) == 0)
    canvas.TextAutoClipped(rc.left, rc.top, value.c_str());
  else {
    PixelScalar canvas_height = rc.bottom - rc.top;
    UPixelScalar text_height = canvas.GetFontHeight();
    PixelScalar top = rc.top + (canvas_height - text_height) / 2;
    canvas.TextAutoClipped(rc.left, top, value.c_str());
  }
}

bool
EditWindow::OnKeyCheck(unsigned key_code) const
{
  switch (key_code) {
  case VK_UP:
    return IsMultiLine() && origin > 0;

  case VK_DOWN:
    return IsMultiLine() && GetRowCount() > GetVisibleRows() &&
      origin < GetRowCount() - GetVisibleRows();
  }

  return false;
}

bool
EditWindow::OnKeyDown(unsigned key_code)
{
  switch (key_code) {
  case VK_UP:
    if (IsMultiLine()) {
      ScrollVertically(-1);
      return true;
    }

    break;

  case VK_DOWN:
    if (IsMultiLine()) {
      ScrollVertically(1);
      return true;
    }

    break;
  }

  return Window::OnKeyDown(key_code);
}

void
EditWindow::SetText(const TCHAR *text)
{
  AssertNoneLocked();

  if (text != NULL)
    value = text;
  else
    value.clear();
  Invalidate();
}
