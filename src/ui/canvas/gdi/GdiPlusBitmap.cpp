// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The XCSoar Project

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
