/*
Copyright_License {

  XCSoar Glide Computer - http://www.xcsoar.org/
  Copyright (C) 2000-2010 The XCSoar Project
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

#ifndef WINUSER_H
#define WINUSER_H

#include <windef.h>

#include <SDL_keysym.h>

enum {
  VK_SPACE = SDLK_SPACE,
  VK_UP = SDLK_UP,
  VK_DOWN = SDLK_DOWN,
  VK_LEFT = SDLK_LEFT,
  VK_RIGHT = SDLK_RIGHT,
  VK_RETURN = SDLK_RETURN,
  VK_F1 = SDLK_F1,
  VK_F2 = SDLK_F2,
  VK_F3 = SDLK_F3,
  VK_F4 = SDLK_F4,
  VK_F5 = SDLK_F5,
  VK_F6 = SDLK_F6,
  VK_F7 = SDLK_F7,
  VK_F8 = SDLK_F8,
  VK_F9 = SDLK_F9,
  VK_F10 = SDLK_F10,
  VK_F11 = SDLK_F11,
  VK_F12 = SDLK_F12,
  VK_ESCAPE = SDLK_ESCAPE,
  VK_TAB = SDLK_TAB,
  VK_BACK = SDLK_BACKSPACE,
  VK_APP1,
  VK_APP2,
  VK_APP3,
  VK_APP4,
  VK_APP5,
  VK_APP6,
  VK_F23,
};

enum {
  IDOK = 1,
  IDCANCEL,
  IDYES,
  IDNO,
  IDRETRY,
  IDABORT,
  IDIGNORE,
};

enum {
  MB_OKCANCEL,
  MB_OK,
  MB_YESNO,
  MB_YESNOCANCEL,
  MB_RETRYCANCEL,
  MB_ABORTRETRYIGNORE,
  MB_ICONINFORMATION = 0x10,
  MB_ICONWARNING = 0x20,
  MB_ICONEXCLAMATION = 0x40,
  MB_ICONQUESTION = 0x80,
  MB_ICONERROR = 0x100,
};

enum {
  DT_EXPANDTABS = 0x1,
  DT_LEFT = 0x2,
  DT_NOCLIP = 0x4,
  DT_WORDBREAK = 0x8,
  DT_CENTER = 0x20,
  DT_NOPREFIX = 0x40,
  DT_VCENTER = 0x80,
  DT_RIGHT = 0x100,
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

static inline bool
IntersectRect(RECT *dest, const RECT *a, const RECT *b)
{
  dest->left = a->left < b->left ? a->left : b->left;
  dest->top = a->top < b->top ? a->top : b->top;
  dest->right = a->right > b->right ? a->right : b->right;
  dest->bottom = a->bottom > b->bottom ? a->bottom : b->bottom;

  return dest->left < dest->right && dest->top < dest->bottom;
}

#endif
