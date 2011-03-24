/*
Copyright_License {

  XCSoar Glide Computer - http://www.xcsoar.org/
  Copyright (C) 2000-2011 The XCSoar Project
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

#include "Screen/Bitmap.hpp"
#include "ResourceLoader.hpp"
#include "OS/PathName.hpp"

#ifdef HAVE_AYGSHELL_DLL
#include "OS/AYGShellDLL.hpp"
#endif

#ifdef HAVE_IMGDECMP_DLL
#include "Screen/RootDC.hpp"
#include "OS/ImgDeCmpDLL.hpp"
#endif

#include <assert.h>

struct DIBINFO : public BITMAPINFO
{
	RGBQUAD	 arColors[255];    // Color table info - adds an extra 255 entries to palette

	operator LPBITMAPINFO()          { return (LPBITMAPINFO) this; }
	operator LPBITMAPINFOHEADER()    { return &bmiHeader;          }
	RGBQUAD* ColorTable()            { return bmiColors;           }
};

bool
Bitmap::load(unsigned id)
{
  reset();

  bitmap = ResourceLoader::LoadBitmap2(id);
  return bitmap != NULL;
}

bool
Bitmap::load_stretch(unsigned id, unsigned zoom)
{
  assert(zoom > 0);

  if (!load(id))
    return false;

  if (zoom <= 1)
    return true;

  PixelSize src_size = get_size();
  PixelSize dest_size;
  dest_size.cx = src_size.cx * zoom;
  dest_size.cy = src_size.cy * zoom;

  HDC dc = ::GetDC(NULL), src_dc = ::CreateCompatibleDC(dc),
    dest_dc = ::CreateCompatibleDC(dc);
  HBITMAP dest_bitmap = ::CreateCompatibleBitmap(dc,
                                                 dest_size.cx, dest_size.cy);
  ::ReleaseDC(NULL, dc);

  if (dest_bitmap == NULL) {
    ::DeleteDC(src_dc);
    ::DeleteDC(dest_dc);
    return false;
  }

  ::SelectObject(src_dc, bitmap);
  ::SelectObject(dest_dc, dest_bitmap);

  ::StretchBlt(dest_dc, 0, 0, dest_size.cx, dest_size.cy,
               src_dc, 0, 0, src_size.cx, src_size.cy,
               SRCCOPY);

  ::DeleteDC(src_dc);
  ::DeleteDC(dest_dc);

  ::DeleteObject(bitmap);
  bitmap = dest_bitmap;

  return true;
}

#ifdef HAVE_IMGDECMP_DLL

static DWORD CALLBACK
imgdecmp_get_data(LPSTR szBuffer, DWORD dwBufferMax, LPARAM lParam)
{
  HANDLE file = (HANDLE)lParam;
  DWORD nbytes = 0;
  return ReadFile(file, szBuffer, dwBufferMax, &nbytes, NULL)
    ? nbytes
    : 0;
}

static HBITMAP
load_imgdecmp_file(const TCHAR *path)
{
  ImgDeCmpDLL imgdecmp_dll;
  if (!imgdecmp_dll.defined())
    return false;

  HANDLE file = ::CreateFile(path, GENERIC_READ, FILE_SHARE_READ, NULL,
                             OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, NULL);
  if (file == INVALID_HANDLE_VALUE)
    return false;

  BYTE buffer[1024];
  HBITMAP bitmap;
  RootDC dc;

  DecompressImageInfo dii;
  dii.dwSize = sizeof(dii);
  dii.pbBuffer = buffer;
  dii.dwBufferMax = sizeof(buffer);
  dii.dwBufferCurrent = 0;
  dii.phBM = &bitmap;
  dii.ppImageRender = NULL;
  dii.iBitDepth = GetDeviceCaps(dc, BITSPIXEL);
  dii.lParam = (LPARAM)file;
  dii.hdc = dc;
  dii.iScale = 100;
  dii.iMaxWidth = 10000;
  dii.iMaxHeight = 10000;
  dii.pfnGetData = imgdecmp_get_data;
  dii.pfnImageProgress = NULL;
  dii.crTransparentOverride = (UINT)-1;

  HRESULT result = imgdecmp_dll.DecompressImageIndirect(&dii);
  ::CloseHandle(file);

  return SUCCEEDED(result)
    ? bitmap
    : NULL;
}

#endif /* HAVE_IMGDECMP_DLL */

bool
Bitmap::load_file(const TCHAR *path)
{
#ifdef HAVE_AYGSHELL_DLL
  AYGShellDLL ayg;
  bitmap = ayg.SHLoadImageFile(path);
  if (bitmap != NULL)
    return true;
#endif

#ifdef HAVE_IMGDECMP_DLL
  bitmap = load_imgdecmp_file(path);
  if (bitmap != NULL)
    return true;
#endif

  return false;
}

void
Bitmap::reset()
{
  if (bitmap != NULL) {
    DeleteObject(bitmap);
    bitmap = NULL;
  }
}

const PixelSize
Bitmap::get_size() const
{
  assert(defined());

  BITMAP bm;
  ::GetObject(bitmap, sizeof(bm), &bm);
  const PixelSize size = { bm.bmWidth, bm.bmHeight };
  return size;
}
