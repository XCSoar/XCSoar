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

#include "Screen/Debug.hpp"

#ifndef NDEBUG

#include <assert.h>

static bool screen_initialized = false;

void
ScreenInitialized()
{
  assert(!screen_initialized);
  screen_initialized = true;
}

void
ScreenDeinitialized()
{
  assert(screen_initialized);
  screen_initialized = false;
}

bool
IsScreenInitialized()
{
  return screen_initialized;
}

#endif
