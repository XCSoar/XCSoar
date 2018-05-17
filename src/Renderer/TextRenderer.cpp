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

#include "TextRenderer.hpp"
#include "Screen/Canvas.hpp"
#include "Screen/AnyCanvas.hpp"
#include "Asset.hpp"

#include <winuser.h>

unsigned
TextRenderer::GetHeight(Canvas &canvas, PixelRect rc,
                        const TCHAR *text) const
{
  return canvas.DrawFormattedText(rc, text, DT_CALCRECT);
}

unsigned
TextRenderer::GetHeight(Canvas &canvas, unsigned width,
                        const TCHAR *text) const
{
  return GetHeight(canvas, PixelRect(0, 0, width, 0), text);
}

unsigned
TextRenderer::GetHeight(const Font &font, unsigned width,
                        const TCHAR *text) const
{
  AnyCanvas canvas;
  canvas.Select(font);
  return GetHeight(canvas, width, text);
}

void
TextRenderer::Draw(Canvas &canvas, PixelRect rc,
                   const TCHAR *text) const
{
  unsigned format = (center ? DT_CENTER : DT_LEFT);

#ifdef USE_GDI
  if (vcenter) {
    const unsigned height = GetHeight(canvas, rc, text);
    int top = (rc.top + rc.bottom - height) / 2;
    if (top > rc.top)
      rc.top = top;
  }
#else
  if (vcenter)
    format |= DT_VCENTER;

  if (control && IsDithered())
    /* button texts are underlined on the Kobo */
    format |= DT_UNDERLINE;
#endif

  canvas.DrawFormattedText(rc, text, format);
}
