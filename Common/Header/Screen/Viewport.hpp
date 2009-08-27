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

#ifndef XCSOAR_SCREEN_VIEWPORT_HPP
#define XCSOAR_SCREEN_VIEWPORT_HPP

#define WIN32_LEAN_AND_MEAN
#include <windows.h>

#if WINDOWSPC > 0
#define HAVE_VIEWPORT
#define HAVE_OFFSET_VIEWPORT
#elif defined(_WIN32_WCE) && _WIN32_WCE >= 0x0500
#define HAVE_VIEWPORT
#if !defined(__GNUC__)
/* OffsetViewportOrgEx() is supported since Windows CE 5.0, but not in
   mingw32ce */
#define HAVE_OFFSET_VIEWPORT
#endif /* __GNUC__ */
#endif

/**
 * This class allows you to get a handle for drawing into an aperture
 * within another drawing handle.
 */
class Viewport {
private:
  HDC hDC;

#ifdef HAVE_VIEWPORT
  POINT old_origin;
#ifndef HAVE_OFFSET_VIEWPORT
  POINT current;
#endif
#else
  /* without the OffsetViewportOrgEx(), we fall back to a virtual
     DC */

  int left, top;
  unsigned width, height;
  HDC hVirtualDC;
  HBITMAP hBitmap;
#endif

public:
#ifdef HAVE_VIEWPORT
  Viewport(HDC _hDC, unsigned _width, unsigned _height)
    :hDC(_hDC) {
    (void)_width;
    (void)_height;

#ifdef HAVE_OFFSET_VIEWPORT
    ::GetViewportOrgEx(hDC, &old_origin);
#else
    ::SetViewportOrgEx(hDC, 0, 0, &old_origin);
    if (old_origin.x != 0 || old_origin.y != 0)
      ::SetViewportOrgEx(hDC, old_origin.x, old_origin.y, NULL);
    current = old_origin;
#endif
  }
#else
  Viewport(HDC _hDC, unsigned _width, unsigned _height)
    :hDC(_hDC), left(0), top(0), width(_width), height(_height)
  {
    hBitmap = ::CreateCompatibleBitmap(hDC, width, height);
    hVirtualDC = ::CreateCompatibleDC(hDC);
    ::SelectObject(hVirtualDC, hBitmap);
  }
#endif

  ~Viewport() {
    restore();
#ifndef HAVE_VIEWPORT
    ::DeleteDC(hVirtualDC);
    ::DeleteObject(hBitmap);
#endif
  }

  /**
   * Restores the previous viewport.
   */
  void restore(void) {
#ifdef HAVE_VIEWPORT
    ::SetViewportOrgEx(hDC, old_origin.x, old_origin.y, NULL);
#endif
  }

  /**
   * Moves the viewport by the specified offset.  The dimensions are
   * not touched.
   */
  void move(int x, int y) {
#ifdef HAVE_VIEWPORT
#ifdef HAVE_OFFSET_VIEWPORT
    ::OffsetViewportOrgEx(hDC, x, y, NULL);
#else /* HAVE_OFFSET_VIEWPORT */
    current.x += x;
    current.y += y;
    ::SetViewportOrgEx(hDC, current.x, current.y, NULL);
#endif /* !HAVE_OFFSET_VIEWPORT */
#else /* HAVE_VIEWPORT */
    left += x;
    top += y;
#endif /* !HAVE_VIEWPORT */
  }

  operator HDC() {
#ifdef HAVE_VIEWPORT
    return hDC;
#else
    return hVirtualDC;
#endif
  }

  /**
   * Call this function to commit your drawings into the real device.
   */
  void commit() {
#ifndef HAVE_VIEWPORT
    ::BitBlt(hDC, left, top, width, height,
             hVirtualDC, 0, 0, SRCCOPY);
#endif
  }
};

#endif
