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

/*! \file
 * \brief Small Windows GDI helper functions
 */

#ifndef XCSOAR_SCREEN_UTIL_HPP
#define XCSOAR_SCREEN_UTIL_HPP

#define WIN32_LEAN_AND_MEAN
#include <windows.h>

enum {
  MAXCLIPPOLYGON = 5000,
};

void ClipPolygon(HDC hdc, const POINT *m_ptin, unsigned int inLength,
                 RECT rc, bool fill);

void ClipPolyline(HDC hdc, POINT* pt, const int npoints, const RECT rc);

void ClipLine(const HDC& hdc, const POINT &ptStart,
              const POINT &ptEnd, const RECT rc);

int  Circle(HDC hdc, long x, long y, int radius, RECT rc, bool clip=false,
            bool fill=true);
int Segment(HDC hdc, long x, long y, int radius, RECT rc,
	    double start,
	    double end,
            bool horizon= false);
// VENTA3 DrawArc
int DrawArc(HDC hdc, long x, long y, int radius, RECT rc,
	    double start,
	    double end);

int
GetTextWidth(HDC hDC, const TCHAR *text);
void
ExtTextOutClip(HDC hDC, int x, int y, const TCHAR *text, int width);

void PolygonRotateShift(POINT* poly, int n, int x, int y,
                        double angle);

#endif
