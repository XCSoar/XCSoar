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

#include "Screen/Brush.hpp"
#include "Screen/Bitmap.hpp"
#include "Screen/Debug.hpp"

#include <assert.h>

void
Brush::Set(const Color c)
{
  assert(IsScreenInitialized());

  #ifndef USE_GDI
  hollow = false;
  color = c;
  #else
  Reset();
  brush = ::CreateSolidBrush(c);
  #endif
}

#ifdef HAVE_HATCHED_BRUSH

void
Brush::Set(const Bitmap &bitmap)
{
  Reset();
  brush = ::CreatePatternBrush(bitmap.native());
}

#endif

void
Brush::Reset()
{
  assert(!IsDefined() || IsScreenInitialized());

  #ifndef USE_GDI
  hollow = true;
  #else
  if (brush != NULL) {
    ::DeleteObject(brush);
    brush = NULL;
  }
  #endif
}
