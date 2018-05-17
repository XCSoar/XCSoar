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

#include "DisplaySize.hpp"
#include "Screen/Point.hpp"

#ifdef WIN32
#include "Screen/GDI/RootDC.hpp"
#include <windows.h>
#elif defined(USE_X11)
#include "Event/Globals.hpp"
#include "Event/Queue.hpp"
#define Font X11Font
#define Window X11Window
#define Display X11Display
#include <X11/Xlib.h>
#undef Font
#undef Window
#undef Display
#endif

PixelSize
Display::GetSize(PixelSize fallback)
{
#ifdef WIN32
  RootDC dc;
  return PixelSize(GetDeviceCaps(dc, HORZRES),
                   GetDeviceCaps(dc, VERTRES));
#elif defined(USE_X11)
  assert(event_queue != nullptr);

  auto display = event_queue->GetDisplay();
  assert(display != nullptr);

  return PixelSize(DisplayWidth(display, 0), DisplayHeight(display, 0));
#else
  /* not implemented: fall back to the main window size (which is
     correct when it's a full-screen window */
  return fallback;
#endif
}
