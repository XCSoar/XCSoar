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

#include "Screen/ProgressWindow.hpp"
#include "Screen/VirtualCanvas.hpp"
#include "resource.h"

#include <algorithm>

using std::min;

ProgressWindow::ProgressWindow(ContainerWindow &parent)
  :background_color(COLOR_WHITE),
   background_brush(background_color),
   position(0)
{
  PixelRect rc = parent.GetClientRect();
  WindowStyle style;
  style.Hide();
  set(parent, rc, style);

  UPixelScalar width = rc.right - rc.left, height = rc.bottom - rc.top;

  // Load progress bar background
  bitmap_progress_border.Load(IDB_PROGRESSBORDER);

  // Determine text height
#ifndef USE_GDI
  font.Set(_T("Droid Sans"), 12);
  text_height = font.GetHeight();
#else
  VirtualCanvas canvas(1, 1);
  text_height = canvas.GetFontHeight();
#endif

  // Make progress bar height proportional to window height
  UPixelScalar progress_height = height / 20;
  UPixelScalar progress_horizontal_border = progress_height / 2;
  progress_border_height = progress_height * 2;

  // Initialize message text field
  TextWindowStyle message_style;
  message_style.center();
  message.set(*this, NULL, 0,
              height - progress_border_height - text_height - (height/48),
              width, text_height, message_style);

#ifndef USE_GDI
  message.SetFont(font);
#endif

  // Initialize progress bar
  ProgressBarStyle pb_style;
  progress_bar.set(*this, progress_horizontal_border,
                   height - progress_border_height + progress_horizontal_border,
                   width - progress_height,
                   progress_height, pb_style);

  message.InstallWndProc(); // needed for on_color()

  // Set progress bar step size and range
  SetRange(0, 1000);
  SetStep(50);

  // Show dialog
  ShowOnTop();
}

void
ProgressWindow::SetMessage(const TCHAR *text)
{
  AssertNoneLocked();
  AssertThread();

  message.set_text(text);
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
  AssertNoneLocked();
  AssertThread();

  if (value == position)
    return;

  position = value;
  progress_bar.SetValue(value);
}

void
ProgressWindow::Step()
{
  progress_bar.Step();
}

void
ProgressWindow::OnResize(UPixelScalar width, UPixelScalar height)
{
  ContainerWindow::OnResize(width, height);

  // Make progress bar height proportional to window height
  UPixelScalar progress_height = height / 20;
  UPixelScalar progress_horizontal_border = progress_height / 2;
  progress_border_height = progress_height * 2;

  if (message.IsDefined())
    message.Move(0, height - progress_border_height - text_height - (height/48),
                 width, text_height);

  if (progress_bar.IsDefined())
    progress_bar.Move(progress_horizontal_border,
                      height - progress_border_height + progress_horizontal_border,
                      width - progress_height,
                      progress_height);

  Invalidate();
}

void
ProgressWindow::OnPaint(Canvas &canvas)
{
  canvas.Clear(background_color);

  // Determine window size
  UPixelScalar window_width = canvas.get_width();
  UPixelScalar window_height = canvas.get_height();

  PixelRect logo_rect;
  logo_rect.left = 0;
  logo_rect.top = 0;
  logo_rect.right = window_width;
  logo_rect.bottom = window_height - progress_border_height;
  logo.draw(canvas, logo_rect);

  // Draw progress bar background
  canvas.Stretch(0, (window_height - progress_border_height),
                 window_width, progress_border_height,
                 bitmap_progress_border);

  ContainerWindow::OnPaint(canvas);
}

const Brush *
ProgressWindow::on_color(Window &window, Canvas &canvas)
{
  canvas.SetTextColor(COLOR_BLACK);
  canvas.SetBackgroundColor(background_color);
  return &background_brush;
}
