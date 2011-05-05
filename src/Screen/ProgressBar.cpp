/*
Copyright_License {

  XCSoar Glide Computer - http://www.xcsoar.org/
  Copyright (C) 2000-2011 The XCSoar Project
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

#include "ProgressBar.hpp"
#include "Thread/Debug.hpp"

#ifndef ENABLE_SDL
#include <commctrl.h>
#else
#include "Screen/Canvas.hpp"
#endif

void
ProgressBarStyle::vertical()
{
#ifndef ENABLE_SDL
  style |= PBS_VERTICAL;
#endif
}

void
ProgressBarStyle::smooth()
{
#ifndef ENABLE_SDL
  style |= PBS_SMOOTH;
#endif
}

void
ProgressBar::set(ContainerWindow &parent,
                 int left, int top, unsigned width, unsigned height,
                 const ProgressBarStyle style)
{
  Window::set(&parent,
#ifndef ENABLE_SDL
              PROGRESS_CLASS, NULL,
#endif
              left, top, width, height,
              style);
}

void
ProgressBar::set_range(unsigned min_value, unsigned max_value)
{
  assert_none_locked();
  assert_thread();

#ifdef ENABLE_SDL
  this->min_value = min_value;
  this->max_value = max_value;
  value = 0;
  step_size = 1;
  expose();
#else /* !ENABLE_SDL */
  ::SendMessage(hWnd,
                PBM_SETRANGE, (WPARAM)0,
                (LPARAM)MAKELPARAM(min_value, max_value));
#endif /* !ENABLE_SDL */
}

void
ProgressBar::set_position(unsigned value)
{
  assert_none_locked();
  assert_thread();

#ifdef ENABLE_SDL
  this->value = value;
  expose();
#else /* !ENABLE_SDL */
  ::SendMessage(hWnd, PBM_SETPOS,
                value, 0);
#endif /* !ENABLE_SDL */
}

void
ProgressBar::set_step(unsigned size)
{
  assert_none_locked();
  assert_thread();

#ifdef ENABLE_SDL
  step_size = size;
  expose();
#else /* !ENABLE_SDL */
  ::SendMessage(hWnd,
                PBM_SETSTEP, (WPARAM)size, (LPARAM)0);
#endif /* !ENABLE_SDL */
}

void
ProgressBar::step()
{
  assert_none_locked();
  assert_thread();

#ifdef ENABLE_SDL
  value += step_size;
  expose();
#else /* !ENABLE_SDL */
  ::SendMessage(hWnd, PBM_STEPIT,
                (WPARAM)0, (LPARAM)0);
#endif /* !ENABLE_SDL */
}

#ifdef ENABLE_SDL
void
ProgressBar::on_paint(Canvas &canvas)
{
  unsigned position = 0;
  if (min_value < max_value) {
    unsigned value = this->value;
    if (value < min_value)
      value = min_value;
    else if (value > max_value)
      value = max_value;
    position = (value - min_value) * get_width() / (max_value - min_value);
  }

  canvas.fill_rectangle(0, 0, position, get_height(), COLOR_GREEN);
  canvas.fill_rectangle(position, 0, get_width(), get_height(), COLOR_WHITE);
}
#endif
