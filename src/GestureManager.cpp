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
#include "Message.hpp"
#include "InputEvents.h"
#include "Screen/Layout.hpp"
#include "Appearance.hpp"
#include "Defines.h"
#include "Simulator.hpp"
#include "TaskClientUI.hpp"
#include "DeviceBlackboard.hpp"
#include "Math/Earth.hpp"
#include "Protection.hpp"
#include "Dialogs.h"
#include "UtilsSystem.hpp"
#include "Compiler.h"

#include <algorithm>

using std::min;
using std::max;

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
getDirection(int X1, int Y1, int X2, int Y2)
{
  int dx = X2 - X1;
  int dy = Y2 - Y1;

  if (dy < 0 && -dy >= abs(dx))
    return _T("U");
  if (dy > 0 && dy >= abs(dx))
    return _T("D");
  if (dx > 0 && dx >= abs(dy))
    return _T("R");
  if (dx < 0 && -dx >= abs(dy))
    return _T("L");

  return _T("");
}

void
GestureManager::AddPoint(int x, int y)
{
  if (!active)
    return;

  if (is_gesture
      || compare_squared(drag_start.x - x, drag_start.y - y,
                         Layout::Scale(70)) == 1) {
    // Set is_gesture = true to save us one square-root operation each call
    is_gesture = true;

    // Get current dragging direction
    const TCHAR* direction = getDirection(drag_last.x, drag_last.y, x, y);

    // If no gesture yet or (the direction has
    // changed and more then 70px from last direction change)...
    if (string_is_empty(gesture)
        || (direction[0] != gesture[_tcslen(gesture) - 1]
            && _tcslen(gesture) < 10
            && compare_squared(gesture_corner.x - x, gesture_corner.y - y,
                               Layout::Scale(70)) == 1)) {
      // Append current direction to the gesture string
      _tcscat(gesture, direction);
      // Save position of the direction change
      gesture_corner.x = x;
      gesture_corner.y = y;
    }

    // Save position for next direction query
    drag_last.x = x;
    drag_last.y = y;
  }
}

void
GestureManager::Start(int x, int y)
{
  active = true;

  drag_start.x = x;
  drag_start.y = y;

  is_gesture = false;
  gesture_corner = drag_last = drag_start;

  // Reset gesture
  _tcscpy(gesture, _T(""));
}

const TCHAR*
GestureManager::Finish()
{
  active = false;

  if (!is_gesture)
    return NULL;

  // Finish gesture
  is_gesture = false;
  return gesture;
}
