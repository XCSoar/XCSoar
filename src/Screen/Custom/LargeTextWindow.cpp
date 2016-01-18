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

#include "Screen/LargeTextWindow.hpp"
#include "Screen/Canvas.hpp"
#include "Screen/Layout.hpp"
#include "Event/KeyCode.hpp"
#include "Util/StringAPI.hxx"

void
LargeTextWindow::Create(ContainerWindow &parent, PixelRect rc,
                        const LargeTextWindowStyle style)
{
  origin = 0;

  Window::Create(&parent, rc, style);
}

unsigned
LargeTextWindow::GetVisibleRows() const
{
  return GetHeight() / GetFont().GetLineSpacing();
}

unsigned
LargeTextWindow::GetRowCount() const
{
  const TCHAR *str = value.c_str();
  unsigned row_count = 1;
  while ((str = StringFind(str, _T('\n'))) != nullptr) {
    str++;
    row_count++;
  }

  return row_count;
}

void
LargeTextWindow::ScrollVertically(int delta_lines)
{
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
LargeTextWindow::OnResize(PixelSize new_size)
{
  Window::OnResize(new_size);

  if (!value.empty()) {
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
LargeTextWindow::OnPaint(Canvas &canvas)
{
  canvas.ClearWhite();

  PixelRect rc(0, 0, canvas.GetWidth() - 1, canvas.GetHeight() - 1);
  canvas.DrawOutlineRectangle(rc.left, rc.top, rc.right, rc.bottom,
                              COLOR_BLACK);

  if (value.empty())
    return;

  const int padding = Layout::GetTextPadding();
  rc.Grow(-padding);

  canvas.SetBackgroundTransparent();
  canvas.SetTextColor(COLOR_BLACK);

  rc.top -= origin * GetFont().GetHeight();

  canvas.Select(GetFont());
  renderer.Draw(canvas, rc, value.c_str());
}

bool
LargeTextWindow::OnKeyCheck(unsigned key_code) const
{
  switch (key_code) {
  case KEY_UP:
    return origin > 0;

  case KEY_DOWN:
    return GetRowCount() > GetVisibleRows() &&
      origin < GetRowCount() - GetVisibleRows();
  }

  return false;
}

bool
LargeTextWindow::OnKeyDown(unsigned key_code)
{
  switch (key_code) {
  case KEY_UP:
    ScrollVertically(-1);
    return true;

  case KEY_DOWN:
    ScrollVertically(1);
    return true;
  }

  return Window::OnKeyDown(key_code);
}

void
LargeTextWindow::SetText(const TCHAR *text)
{
  if (text != nullptr)
    value = text;
  else
    value.clear();
  Invalidate();
}
