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

#ifndef WINUSER_H
#define WINUSER_H

#include <windef.h>

enum {
  VK_SPACE,
  VK_APP1,
  VK_APP2,
  VK_APP3,
  VK_APP4,
  VK_APP5,
  VK_APP6,
  VK_UP,
  VK_DOWN,
  VK_LEFT,
  VK_RIGHT,
  VK_RETURN,
  VK_F1,
  VK_F2,
  VK_F3,
  VK_F4,
  VK_F5,
  VK_F6,
  VK_F7,
  VK_F8,
  VK_F9,
  VK_F10,
  VK_F11,
  VK_F12,
  VK_F23,
  VK_ESCAPE,
};

enum {
  WM_ERASEBKGND,
  WM_PAINT,
  WM_SIZE,
  WM_WINDOWPOSCHANGED,
  WM_CREATE,
  WM_DESTROY,
  WM_LBUTTONDBLCLK,
  WM_LBUTTONDOWN,
  WM_COMMAND,
  WM_LBUTTONUP,
  WM_KEYUP,
  WM_KEYDOWN,
  WM_CLOSE,
  WM_SETFONT,
  WM_TIMER,
  WM_INITMENUPOPUP,
  WM_SETFOCUS,
  WM_ACTIVATE,
  WM_SETTINGCHANGE,
};

enum {
  DT_EXPANDTABS = 0x1,
  DT_LEFT = 0x2,
  DT_NOCLIP = 0x4,
  DT_WORDBREAK = 0x8,
  DT_CALCRECT = 0x10,
  DT_CENTER = 0x20,
};

static inline void
SetRect(RECT *rc, int left, int top, int right, int bottom)
{
  rc->left = left;
  rc->top = top;
  rc->right = right;
  rc->bottom = bottom;
}

static inline void
SetRectEmpty(RECT *rc)
{
  rc->left = 0;
  rc->top = 0;
  rc->right = 0;
  rc->bottom = 0;
}

static inline void
CopyRect(RECT *dest, const RECT *src)
{
  *dest = *src;
}

static inline void
InflateRect(RECT *rc, int dx, int dy)
{
  rc->left -= dx;
  rc->top -= dy;
  rc->right += dx;
  rc->bottom += dy;
}

static inline void
OffsetRect(RECT *rc, int dx, int dy)
{
  rc->left += dx;
  rc->top += dy;
  rc->right += dx;
  rc->bottom += dy;
}

static inline bool
PtInRect(const RECT *rc, POINT pt)
{
  return pt.x >= rc->left && pt.x < rc->right &&
    pt.y >= rc->top && pt.y < rc->bottom;
}

static inline bool
EqualRect(const RECT *a, const RECT *b)
{
  return a->left == b->left && a->top == b->top &&
    a->right == b->right && a->bottom == b->bottom;
}

#endif
