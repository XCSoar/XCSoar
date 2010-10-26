/*
Copyright_License {

  XCSoar Glide Computer - http://www.xcsoar.org/
  Copyright (C) 2000, 2001, 2002, 2003, 2004, 2005, 2006, 2007, 2008, 2009

	M Roberts (original release)
	Robin Birch <robinb@ruffnready.co.uk>
	Samuel Gisiger <samuel.gisiger@triadis.ch>
	Jeff Goodenough <jeff@enborne.f2s.com>
	Alastair Harrison <aharrison@magic.force9.co.uk>
	Scott Penrose <scottp@dd.com.au>
	John Wharington <jwharington@gmail.com>
	Lars H <lars_hn@hotmail.com>
	Rob Dunning <rob@raspberryridgesheepfarm.com>
	Russell King <rmk@arm.linux.org.uk>
	Paolo Ventafridda <coolwind@email.it>
	Tobias Lohner <tobias@lohner-net.de>
	Mirek Jezek <mjezek@ipplc.cz>
	Max Kellermann <max@duempel.org>
	Tobias Bieniek <tobias.bieniek@gmx.de>

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

static const TCHAR*const PROGRESS_CLASS = NULL;
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
  Window::set(&parent, PROGRESS_CLASS, NULL,
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

  canvas.fill_rectangle(0, 0, position, get_height(), Color::GREEN);
  canvas.fill_rectangle(position, 0, get_width(), get_height(), Color::WHITE);
}
#endif
