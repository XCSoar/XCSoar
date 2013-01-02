/*
Copyright_License {

  XCSoar Glide Computer - http://www.xcsoar.org/
  Copyright (C) 2000-2013 The XCSoar Project
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

#ifndef NDEBUG
#include "Util/UTF8.hpp"
#endif

#include <algorithm>
#include <assert.h>
#include <string.h>
#include <winuser.h>

void
Canvas::DrawButton(PixelRect rc, bool down)
{
  const Pen old_pen = pen;

  Color gray = COLOR_LIGHT_GRAY;
  DrawFilledRectangle(rc, gray);

  Pen bright(1, LightColor(gray));
  Pen dark(1, DarkColor(gray));

  Select(down ? dark : bright);
  DrawTwoLinesExact(rc.left, rc.bottom - 2, rc.left, rc.top,
                    rc.right - 2, rc.top);
  DrawTwoLinesExact(rc.left + 1, rc.bottom - 3, rc.left + 1, rc.top + 1,
                    rc.right - 3, rc.top + 1);

  Select(down ? bright : dark);
  DrawTwoLinesExact(rc.left + 1, rc.bottom - 1, rc.right - 1, rc.bottom - 1,
                    rc.right - 1, rc.top + 1);
  DrawTwoLinesExact(rc.left + 2, rc.bottom - 2, rc.right - 2, rc.bottom - 2,
                    rc.right - 2, rc.top + 2);

  pen = old_pen;
}

const PixelSize
Canvas::CalcTextSize(const TCHAR *text, size_t length) const
{
  assert(text != NULL);

  TCHAR *duplicated = _tcsdup(text);
  duplicated[length] = 0;

#ifndef UNICODE
  assert(ValidateUTF8(duplicated));
#endif

  const PixelSize size = CalcTextSize(duplicated);
  free(duplicated);

  return size;
}

void
Canvas::DrawFormattedText(PixelRect *rc, const TCHAR *text, unsigned format)
{
  assert(text != NULL);
#ifndef UNICODE
  assert(ValidateUTF8(text));
#endif

  if (font == NULL)
    return;

  UPixelScalar skip = font->GetLineSpacing();
  unsigned max_lines = (format & DT_CALCRECT) ? -1 :
                       (rc->bottom - rc->top + skip - 1) / skip;

  size_t len = _tcslen(text);
  TCHAR *duplicated = new TCHAR[len + 1], *p = duplicated;
  unsigned lines = 1;
  for (const TCHAR *i = text; *i != _T('\0'); ++i) {
    TCHAR ch = *i;
    if (ch == _T('\n')) {
      /* explicit line break */

      if (++lines >= max_lines)
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
  if (format & DT_WORDBREAK) {
    for (size_t i = 0; i < len; i += _tcslen(duplicated + i) + 1) {
      PixelSize sz = CalcTextSize(duplicated + i);
      TCHAR *prev_p = NULL;

      // remove words from behind till line fits or no more space is found
      while (sz.cx > rc->right - rc->left &&
             (p = _tcsrchr(duplicated + i, _T(' '))) != NULL) {
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
  }

  if (format & DT_CALCRECT) {
    rc->bottom = rc->top + lines * skip;
    delete[] duplicated;
    return;
  }

  PixelScalar y = (format & DT_VCENTER) && lines < max_lines
    ? (PixelScalar)(rc->top + rc->bottom - lines * skip) / 2
    : rc->top;
  for (size_t i = 0; i < len; i += _tcslen(duplicated + i) + 1) {
    if (duplicated[i] != _T('\0')) {
      PixelScalar x;
      if (format & (DT_RIGHT | DT_CENTER)) {
        PixelSize sz = CalcTextSize(duplicated + i);
        x = (format & DT_CENTER) ? (rc->left + rc->right - sz.cx)/2 :
                                    rc->right - sz.cx;  // DT_RIGHT
      } else {  // default is DT_LEFT
        x = rc->left;
      }

      TextAutoClipped(x, y, duplicated + i);
    }
    y += skip;
    if (y >= rc->bottom)
      break;
  }

  delete[] duplicated;
}

void
Canvas::DrawText(PixelScalar x, PixelScalar y,
                 const TCHAR *_text, size_t length)
{
  assert(_text != NULL);

  TCHAR copy[length + 1];
  std::copy(_text, _text + length, copy);
  copy[length] = _T('\0');

#ifndef UNICODE
  assert(ValidateUTF8(copy));
#endif

  DrawText(x, y, copy);
}

void
Canvas::DrawOpaqueText(PixelScalar x, PixelScalar y, const PixelRect &rc,
                       const TCHAR *_text)
{
  assert(_text != NULL);
#ifndef UNICODE
  assert(ValidateUTF8(_text));
#endif

  DrawFilledRectangle(rc, background_color);
  DrawTransparentText(x, y, _text);
}
