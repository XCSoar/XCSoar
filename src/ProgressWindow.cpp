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

#include "ProgressWindow.hpp"
#include "Screen/Layout.hpp"
#include "Look/FontDescription.hpp"
#include "Resources.hpp"

#ifdef USE_WINUSER
#include "Screen/AnyCanvas.hpp"
#else
#include "Screen/Canvas.hpp"
#endif

ProgressWindow::ProgressWindow(ContainerWindow &parent)
  :background_color(COLOR_WHITE)
{
  message.clear();

  PixelRect rc = parent.GetClientRect();
  WindowStyle style;
  style.Hide();
  Create(parent, rc, style);

  // Load progress bar background
  bitmap_progress_border.Load(IDB_PROGRESSBORDER);

  // Determine text height
#ifndef USE_WINUSER
  font.Load(FontDescription(Layout::FontScale(10)));
  text_height = font.GetHeight();
#else
  {
    AnyCanvas canvas;
    text_height = canvas.GetFontHeight();
  }
#endif

  UpdateLayout(rc);

  // Initialize progress bar
  progress_bar.Create(*this, progress_bar_position);

  // Set progress bar step size and range
  SetRange(0, 1000);
  SetStep(50);

  // Show dialog
  ShowOnTop();
}

void
ProgressWindow::UpdateLayout(PixelRect rc)
{
  const unsigned height = rc.GetHeight();

  // Make progress bar height proportional to window height
  const unsigned progress_height = height / 20;
  const unsigned progress_horizontal_border = progress_height / 2;
  const unsigned progress_border_height = progress_height * 2;

  logo_position = rc;
  logo_position.bottom -= progress_border_height;

  message_position = rc;
  message_position.bottom -= progress_border_height + height / 48;
  message_position.top = message_position.bottom - text_height;

  bottom_position = rc;
  bottom_position.top = bottom_position.bottom - progress_border_height;

  progress_bar_position.left = bottom_position.left + progress_horizontal_border;
  progress_bar_position.right = bottom_position.right - progress_horizontal_border;
  progress_bar_position.top = bottom_position.top + progress_horizontal_border;
  progress_bar_position.bottom = bottom_position.bottom - progress_horizontal_border;
}

void
ProgressWindow::SetMessage(const TCHAR *text)
{
  AssertThread();

  if (text == nullptr)
    text = _T("");

  message = text;
  Invalidate(message_position);
}

void
ProgressWindow::SetRange(unsigned min_value, unsigned max_value)
{
  progress_bar.SetRange(min_value, max_value);
}

void
ProgressWindow::SetStep(unsigned size)
{
  progress_bar.SetStep(size);
}

void
ProgressWindow::SetValue(unsigned value)
{
  AssertThread();

  progress_bar.SetValue(value);
}

void
ProgressWindow::Step()
{
  progress_bar.Step();
}

void
ProgressWindow::OnResize(PixelSize new_size)
{
  ContainerWindow::OnResize(new_size);

  UpdateLayout(GetClientRect());

  if (progress_bar.IsDefined())
    progress_bar.Move(progress_bar_position);

  Invalidate();
}

void
ProgressWindow::OnPaint(Canvas &canvas)
{
  canvas.Clear(background_color);

  logo.draw(canvas, logo_position);

  // Draw progress bar background
  canvas.Stretch(bottom_position.left, bottom_position.top,
                 bottom_position.GetWidth(),
                 bottom_position.GetHeight(),
                 bitmap_progress_border);

#ifndef USE_WINUSER
  canvas.Select(font);
#endif
  canvas.SetBackgroundTransparent();
  canvas.SetTextColor(COLOR_BLACK);
  canvas.DrawText((message_position.left + message_position.right
                   - canvas.CalcTextWidth(message.c_str())) / 2,
                  message_position.top,
                  message.c_str());

  ContainerWindow::OnPaint(canvas);
}
