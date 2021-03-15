/*
Copyright_License {

  XCSoar Glide Computer - http://www.xcsoar.org/
  Copyright (C) 2000-2021 The XCSoar Project
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

#include "GdiPlusBitmap.hpp"

#if defined(_MSC_VER)
# include <algorithm>
using std::min;  // to avoid the missing 'min' in the gdiplush headers
using std::max;  // to avoid the missing 'max' in the gdiplush headers
#endif  // _MSC_VER

#include <assert.h>
#include <unknwn.h>
#include <gdiplus.h>

static ULONG_PTR gdiplusToken;

//----------------------------------------------------------------------------
void
GdiStartup()
{
  Gdiplus::GdiplusStartupInput gdiplusStartupInput;
  Gdiplus::GdiplusStartup(&gdiplusToken, &gdiplusStartupInput, NULL);
}

//----------------------------------------------------------------------------
void
GdiShutdown()
{
  Gdiplus::GdiplusShutdown(gdiplusToken);
}

//----------------------------------------------------------------------------
// can load: BMP, GIF, JPEG, PNG, TIFF, Exif, WMF, and EMF
HBITMAP
GdiLoadImage(const TCHAR* filename)
{
  HBITMAP result = nullptr;
#ifdef _UNICODE  // TCHAR has to be WCHAR in GdiPlus
  Gdiplus::Bitmap bitmap(filename, false);
  if (bitmap.GetLastStatus() != Gdiplus::Ok)
    return nullptr;
  const Gdiplus::Color color = Gdiplus::Color::White;
  if (bitmap.GetHBITMAP(color, &result) != Gdiplus::Ok)
    return nullptr;
#endif  // _UNICODE
  return result;
}
