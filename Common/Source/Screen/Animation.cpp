/*
Copyright_License {

  XCSoar Glide Computer - http://www.xcsoar.org/
  Copyright (C) 2000 - 2009

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

#include "Screen/Animation.hpp"

#ifndef WINDOWSPC
#define GdiFlush() do { } while (0)
#endif

bool EnableAnimation = false;

static RECT AnimationRectangle = {0,0,0,0};

void SetSourceRectangle(RECT fromRect) {
  AnimationRectangle = fromRect;
}

RECT WINAPI DrawWireRects(LPRECT lprcTo, UINT nMilliSecSpeed)
{
  if (!EnableAnimation)
    return AnimationRectangle;

  LPRECT lprcFrom = &AnimationRectangle;
  const int nNumSteps = 10;

  GdiFlush();
  Sleep(10);  // Let the desktop window sort itself out

  // if hwnd is null - "you have the CON".
  HDC hDC = ::GetDC(NULL);

  // Pen size, urmmm not too thick
  HPEN hPen = ::CreatePen(PS_SOLID, 2, RGB(0,0,0));

  int nMode = ::SetROP2(hDC, R2_NOT);
  HPEN hOldPen = (HPEN) ::SelectObject(hDC, hPen);

  for (int i = 0; i < nNumSteps; i++)
    {
      double dFraction = (double) i / (double) nNumSteps;

      RECT transition;
      transition.left   = lprcFrom->left +
	(int)((lprcTo->left - lprcFrom->left) * dFraction);
      transition.right  = lprcFrom->right +
	(int)((lprcTo->right - lprcFrom->right) * dFraction);
      transition.top    = lprcFrom->top +
	(int)((lprcTo->top - lprcFrom->top) * dFraction);
      transition.bottom = lprcFrom->bottom +
	(int)((lprcTo->bottom - lprcFrom->bottom) * dFraction);

      POINT pt[5];
      pt[0].x = transition.left; pt[0].y= transition.top;
      pt[1].x = transition.right; pt[1].y= transition.top;
      pt[2].x = transition.right; pt[2].y= transition.bottom;
      pt[3].x = transition.left; pt[3].y= transition.bottom;
      pt[4].x = transition.left; pt[4].y= transition.top;

      // We use Polyline because we can determine our own pen size
      // Draw Sides
      ::Polyline(hDC,pt,5);

      GdiFlush();

      Sleep(nMilliSecSpeed);

      // UnDraw Sides
      ::Polyline(hDC,pt,5);

      GdiFlush();
    }

  ::SetROP2(hDC, nMode);
  ::SelectObject(hDC, hOldPen);
  ::DeleteObject(hPen);
  ::ReleaseDC(NULL,hDC);
  return AnimationRectangle;
}
