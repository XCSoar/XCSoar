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

#include <algorithm>

int
compare_squared(int a, int b, int c)
{
  int a2b2 = a*a+b*b;
  int c2 = c*c;
  if (a2b2 > c2)
    return 1;
  if (a2b2 < c2)
    return -1;
  return 0;
}

gcc_const const TCHAR*
getDirection(int dx, int dy)
{
  if (dy < 0 && -dy >= abs(dx) * 2)
    return _T("U");
  if (dy > 0 && dy >= abs(dx) * 2)
    return _T("D");
  if (dx > 0 && dx >= abs(dy) * 2)
    return _T("R");
  if (dx < 0 && -dx >= abs(dy) * 2)
    return _T("L");

  return NULL;
}

void
GestureManager::AddPoint(int x, int y)
{
  if (!active)
    return;

  // Get current dragging direction
  int dx = x - drag_last.x;
  int dy = y - drag_last.y;

  if (compare_squared(dx, dy, Layout::Scale(20)) != 1)
    return;

  // Save position for next direction query
  drag_last.x = x;
  drag_last.y = y;

  const TCHAR* direction = getDirection(dx, dy);
  if (!direction)
    return;

  if (direction[0] == gesture[_tcslen(gesture) - 1])
    return;

  if (_tcslen(gesture) < 10)
    _tcscat(gesture, direction);
}

void
GestureManager::Start(int x, int y)
{
  active = true;

  drag_last.x = x;
  drag_last.y = y;

  // Reset gesture
  _tcscpy(gesture, _T(""));
}

const TCHAR*
GestureManager::Finish()
{
  active = false;

  return gesture;
}
