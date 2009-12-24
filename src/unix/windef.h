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

#ifndef WINDEF_H
#define WINDEF_H

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>
#include <wchar.h>
#include <stdarg.h>
#include <time.h>
#include <unistd.h>

#include "tchar.h"

#ifndef MAX_PATH
#define MAX_PATH 4096
#endif

#define _hypot hypot

typedef bool BOOL;
enum {
	FALSE = false,
	TRUE = true,
};

typedef char CHAR;
typedef uint8_t BYTE;
typedef int64_t __int64;
typedef int64_t _int64;
typedef uint32_t DWORD;
typedef DWORD *LPDWORD;
typedef int32_t LONG;
typedef uint32_t ULONG;
typedef uint32_t WORD;
typedef unsigned int UINT;
typedef int INT;
typedef int32_t WPARAM;
typedef int32_t LPARAM;
typedef int32_t LRESULT;
typedef void *LPVOID;

typedef struct w32_handle *HANDLE;
typedef DWORD HRESULT;
typedef HANDLE HINSTANCE;
typedef HANDLE HBRUSH;
typedef HANDLE HPEN;
typedef HANDLE HBITMAP;
typedef HANDLE HFONT;
typedef HANDLE HCURSOR;
typedef HANDLE HICON;
typedef HANDLE ATOM;
typedef struct w32_wnd *HWND;
typedef struct w32_dc *HDC;

#define INVALID_HANDLE_VALUE NULL
#define HWND_TOP NULL

enum {
  ERROR_SUCCESS = 0,
};

typedef DWORD COLORREF;
typedef DWORD* LPCOLORREF;

static inline COLORREF
RGB(int r, int g, int b)
{
  return (r & 0xff) | ((g & 0xff) << 8) | ((b & 0xff) << 16);
}

typedef struct tagPOINT {
	LONG x;
	LONG y;
} POINT;

typedef struct tagSIZE {
  LONG cx;
  LONG cy;
} SIZE, *PSIZE;

typedef struct _RECT {
	LONG left;
	LONG top;
	LONG right;
	LONG bottom;
} RECT;

typedef RECT *LPRECT;

#define WINAPI
#define _cdecl
#define CALLBACK

#define _wcsdup wcsdup

#define LF_FACESIZE 256

typedef struct tagLOGFONT {
	LONG lfHeight;
	LONG lfWidth;
	LONG lfEscapement;
	LONG lfOrientation;
	LONG lfWeight;
	BYTE lfItalic;
	BYTE lfUnderline;
	BYTE lfStrikeOut;
	BYTE lfCharSet;
	BYTE lfOutPrecision;
	BYTE lfClipPrecision;
	BYTE lfQuality;
	BYTE lfPitchAndFamily;
	TCHAR lfFaceName[LF_FACESIZE];
} LOGFONT;

typedef struct {
	HWND hwnd;
	UINT message;
	WPARAM wParam;
	LPARAM lParam;
	DWORD time;
	POINT pt;
} MSG, *PMSG;

typedef LRESULT CALLBACK (*WNDPROC)(
    HWND hwnd,
    UINT uMsg,
    WPARAM wParam,
    LPARAM lParam
);

typedef void *LPSECURITY_ATTRIBUTES;

typedef struct OVERLAPPED OVERLAPPED;
typedef OVERLAPPED *LPOVERLAPPED;

#endif
