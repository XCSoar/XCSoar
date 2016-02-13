/*
Copyright_License {

  XCSoar Glide Computer - http://www.xcsoar.org/
  Copyright (C) 2000-2016 The XCSoar Project
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

#include "Screen/Canvas.hpp"
#include "Util/StringAPI.hxx"

#ifndef NDEBUG
#include "Util/UTF8.hpp"
#endif

#include <algorithm>
#include <assert.h>
#include <string.h>
#include <winuser.h>

const PixelSize
Canvas::CalcTextSize(const TCHAR *text, size_t length) const
{
  assert(text != nullptr);

  TCHAR *duplicated = _tcsdup(text);
  duplicated[length] = 0;

#ifndef UNICODE
  assert(ValidateUTF8(duplicated));
#endif

  const PixelSize size = CalcTextSize(duplicated);
  free(duplicated);

  return size;
}

unsigned
Canvas::DrawFormattedText(const PixelRect r, const TCHAR *text,
                          unsigned format)
{
  assert(text != nullptr);
#ifndef UNICODE
  assert(ValidateUTF8(text));
#endif

  if (font == nullptr)
    return 0;

  unsigned skip = font->GetLineSpacing();
  unsigned max_lines = (format & DT_CALCRECT) ? -1 :
    (r.GetHeight() + skip - 1) / skip;

  size_t len = _tcslen(text);
  TCHAR *duplicated = new TCHAR[len + 1], *p = duplicated;
  unsigned lines = 1;
  for (const TCHAR *i = text; *i != _T('\0'); ++i) {
    TCHAR ch = *i;
    if (ch == _T('\n')) {
      /* explicit line break */

      if (++lines > max_lines)
        break;

      ch = _T('\0');
    } else if (ch == _T('\r'))
      /* skip */
      continue;
    else if ((unsigned)ch < 0x20)
      /* replace non-printable characters */
      ch = _T(' ');

    *p++ = ch;
  }

  *p = _T('\0');
  len = p - duplicated;

  // simple wordbreak algorithm. looks for single spaces only, no tabs,
  // no grouping of multiple spaces
  for (size_t i = 0; i < len; i += _tcslen(duplicated + i) + 1) {
    PixelSize sz = CalcTextSize(duplicated + i);
    TCHAR *prev_p = nullptr;

    // remove words from behind till line fits or no more space is found
    while (unsigned(sz.cx) > r.GetWidth() &&
           (p = StringFindLast(duplicated + i, _T(' '))) != nullptr) {
      if (prev_p)
        *prev_p = _T(' ');
      *p = _T('\0');
      prev_p = p;
      sz = CalcTextSize(duplicated + i);
    }

    if (prev_p) {
      lines++;
      if (lines >= max_lines)
        break;
    }
  }

  if (format & DT_CALCRECT) {
    delete[] duplicated;
    return lines * skip;
  }

  int y = (format & DT_VCENTER) && lines < max_lines
    ? (r.top + r.bottom - lines * skip) / 2
    : r.top;
  for (size_t i = 0; i < len; i += _tcslen(duplicated + i) + 1) {
    if (duplicated[i] != _T('\0')) {
      int x;
      if (format & (DT_RIGHT | DT_CENTER)) {
        PixelSize sz = CalcTextSize(duplicated + i);
        x = (format & DT_CENTER)
          ? (r.left + r.right - sz.cx) / 2
          : r.right - sz.cx;  // DT_RIGHT
      } else {  // default is DT_LEFT
        x = r.left;
      }

      TextAutoClipped(x, y, duplicated + i);

      if (format & DT_UNDERLINE)
        DrawHLine(x, x + CalcTextWidth(duplicated + i),
                  y + font->GetAscentHeight() + 1, text_color);
    }
    y += skip;
    if (y >= r.bottom)
      break;
  }

  delete[] duplicated;
  return lines * skip;
}

void
Canvas::DrawText(int x, int y,
                 const TCHAR *_text, size_t length)
{
  assert(_text != nullptr);

  TCHAR copy[length + 1];
  *std::copy_n(_text, length, copy) = _T('\0');

#ifndef UNICODE
  assert(ValidateUTF8(copy));
#endif

  DrawText(x, y, copy);
}

void
Canvas::DrawOpaqueText(int x, int y, const PixelRect &rc,
                       const TCHAR *_text)
{
  assert(_text != nullptr);
#ifndef UNICODE
  assert(ValidateUTF8(_text));
#endif

  DrawFilledRectangle(rc, background_color);
  DrawTransparentText(x, y, _text);
}
