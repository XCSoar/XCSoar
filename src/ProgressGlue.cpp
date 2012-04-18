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

#include "ProgressGlue.hpp"
#include "Screen/SingleWindow.hpp"
#include "Screen/ProgressWindow.hpp"
#include "Interface.hpp"
#include "UIGlobals.hpp"
#include "PeriodClock.hpp"

static ProgressWindow *global_progress_window;

/**
 * This clock throttles screen updates.
 */
static PeriodClock throttle_clock;

void
ProgressGlue::Create(const TCHAR *text)
{
  UIGlobals::GetMainWindow().RefreshSize();

  if (global_progress_window == NULL)
    global_progress_window = new ProgressWindow(UIGlobals::GetMainWindow());

  global_progress_window->set_message(text);
  global_progress_window->set_pos(0);

  UIGlobals::GetMainWindow().refresh();
  throttle_clock.Reset();
}

void
ProgressGlue::Resize(UPixelScalar width, UPixelScalar height)
{
  if (global_progress_window == NULL)
    return;

  global_progress_window->Move(0, 0, width, height);
  throttle_clock.Reset();
}

void
ProgressGlue::Close()
{
  delete global_progress_window;
  global_progress_window = NULL;
}

void
ProgressGlue::Step()
{
  if (global_progress_window == NULL)
    return;

  if (!throttle_clock.CheckUpdate(200))
    return;

  global_progress_window->step();
  UIGlobals::GetMainWindow().RefreshSize();
  UIGlobals::GetMainWindow().refresh();
}

void
ProgressGlue::SetValue(unsigned value)
{
  if (global_progress_window == NULL)
    return;

  if (!throttle_clock.CheckUpdate(200))
    return;

  global_progress_window->set_pos(value);
  UIGlobals::GetMainWindow().RefreshSize();
  UIGlobals::GetMainWindow().refresh();
}

void
ProgressGlue::SetRange(unsigned value)
{
  if (global_progress_window == NULL)
    return;

  global_progress_window->set_range(0, value);
  throttle_clock.Reset();
}

void
ProgressGlue::SetStep(int step)
{
  if (global_progress_window == NULL)
    return;

  global_progress_window->set_step(step);
  throttle_clock.Reset();
}
