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

#include "TerminalWindow.hpp"
#include "Screen/Canvas.hpp"
#include "Look/TerminalLook.hpp"
#include "Util/CharUtil.hpp"

void
TerminalWindow::Write(const char *p, size_t length)
{
  const char *end = p + length;
  while (p < end) {
    char ch = *p++;

    if (ch == '\n')
      NewLine();
    else if (ch == '\r')
      continue;
    else {
      if (!IsPrintableASCII(ch))
        ch = '.';
      data.Get(cursor_x, cursor_y) = ch;
      Advance();
    }
  }

  Invalidate();
}

void
TerminalWindow::Clear()
{
  cursor_x = cursor_y = 0;
  std::fill(data.begin(), data.end(), ' ');
  Invalidate();
}

void
TerminalWindow::Scroll()
{
#ifdef ANDROID
  std::copy(data.GetPointerAt(0, 1), data.end(), data.begin());
#else
  std::move(data.GetPointerAt(0, 1), data.end(), data.begin());
#endif

  auto end = data.end();
  std::fill(end - data.GetWidth(), end, ' ');

  Invalidate();
}

void
TerminalWindow::NewLine()
{
  cursor_x = 0;
  ++cursor_y;

  if (cursor_y >= data.GetHeight()) {
    cursor_y = data.GetHeight() - 1;
    Scroll();
  }
}

void
TerminalWindow::Advance()
{
  ++cursor_x;

  if (cursor_x >= data.GetWidth())
    NewLine();
}

void
TerminalWindow::OnCreate()
{
  PaintWindow::OnCreate();
  cell_size = look.font.TextSize(_T("W"));
  cursor_x = 0;
  cursor_y = 0;
  data.Reset();
}

void
TerminalWindow::OnResize(PixelSize new_size)
{
  PaintWindow::OnResize(new_size);

  data.GrowPreserveFill(std::max(1u, unsigned(new_size.cx / cell_size.cx)),
                        std::max(1u, unsigned(new_size.cy / cell_size.cy)),
                        ' ');
  if (cursor_x >= data.GetWidth())
    cursor_x = data.GetWidth() - 1;
  if (cursor_y >= data.GetHeight())
    cursor_y = data.GetHeight() - 1;

  Invalidate();
}

void
TerminalWindow::OnPaint(Canvas &canvas)
{
  OnPaint(canvas, GetClientRect());
}

void
TerminalWindow::OnPaint(Canvas &canvas, const PixelRect &p_dirty)
{
  canvas.SetBackgroundTransparent();
  canvas.SetTextColor(look.text_color);
  canvas.Select(look.font);

  const PixelRect cell_dirty = {
    p_dirty.left / cell_size.cx,
    p_dirty.top / cell_size.cy,
    std::min(p_dirty.right / cell_size.cx + 1,
             int(data.GetWidth())),
    std::min(p_dirty.bottom / cell_size.cy + 1,
             int(data.GetHeight())),
  };

  const int x(cell_dirty.left * cell_size.cx);
  const size_t length = cell_dirty.GetWidth();

  auto text = data.GetPointerAt(cell_dirty.left, cell_dirty.top);
  for (int cell_y = cell_dirty.top, p_y = cell_y * cell_size.cy;
       cell_y < cell_dirty.bottom;
       ++cell_y, p_y += cell_size.cy, text += data.GetWidth()) {
    canvas.DrawFilledRectangle(p_dirty.left, p_y,
                          p_dirty.right, p_y + cell_size.cy,
                          look.background_color);
    canvas.DrawText(x, p_y, text, length);
  }

  int cell_bottom_y(cell_dirty.bottom * cell_size.cy);
  if (cell_bottom_y < p_dirty.bottom)
    canvas.DrawFilledRectangle(p_dirty.left, cell_bottom_y,
                          p_dirty.right, p_dirty.bottom,
                          look.background_color);
}
