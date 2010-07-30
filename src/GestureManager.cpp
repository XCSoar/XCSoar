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

#include "GestureManager.hpp"
#include "Screen/Layout.hpp"
#include "StringUtil.hpp"
#include "Math/FastMath.h"
#include "Compiler.h"

#include <algorithm>

static gcc_const TCHAR
getDirection(int dx, int dy)
{
  if (dy < 0 && -dy >= abs(dx) * 2)
    return _T('U');
  if (dy > 0 && dy >= abs(dx) * 2)
    return _T('D');
  if (dx > 0 && dx >= abs(dy) * 2)
    return _T('R');
  if (dx < 0 && -dx >= abs(dy) * 2)
    return _T('L');

  return _T('\0');
}

void
GestureManager::AddPoint(int x, int y)
{
  if (!active)
    return;

  // Calculate deltas
  int dx = x - drag_last.x;
  int dy = y - drag_last.y;

  // See if we've reached the threshold already
  if (compare_squared(dx, dy, Layout::Scale(20)) != 1)
    return;

  // Save position for next call
  drag_last.x = x;
  drag_last.y = y;

  // Get current dragging direction
  TCHAR direction = getDirection(dx, dy);

  // Return if we are in an unclear direction
  if (direction == _T('\0'))
    return;

  // Return if we are still in the same direction
  size_t length = _tcslen(gesture);
  if (length < sizeof(gesture) / sizeof(gesture[0]) - 1 &&
      gesture[length - 1] != direction) {
    gesture[length] = direction;
    gesture[length + 1] = _T('\0');
  }
}

void
GestureManager::Start(int x, int y)
{
  active = true;

  // Reset last position
  drag_last.x = x;
  drag_last.y = y;

  // Reset gesture
  _tcscpy(gesture, _T(""));
}

const TCHAR*
GestureManager::Finish()
{
  active = false;

  if (string_is_empty(gesture))
    return NULL;

  return gesture;
}
