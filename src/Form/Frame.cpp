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

#include "Form/Frame.hpp"
#include "Screen/AnyCanvas.hpp"
#include "Screen/Layout.hpp"
#include "Look/DialogLook.hpp"

#include <winuser.h>

WndFrame::WndFrame(ContainerWindow &parent, const DialogLook &_look,
                   PixelRect rc,
                   const WindowStyle style)
  :look(_look),
   caption_color(look.text_color),
   mCaptionStyle(DT_LEFT | DT_NOCLIP | DT_WORDBREAK)
{
  text.clear();

  Create(parent, rc, style);
}

void
WndFrame::SetAlignCenter()
{
  mCaptionStyle &= ~(DT_LEFT|DT_RIGHT);
  mCaptionStyle |= DT_CENTER;
  Invalidate();
}

void
WndFrame::SetVAlignCenter()
{
  mCaptionStyle |= DT_VCENTER;
  Invalidate();
}

void
WndFrame::SetText(const TCHAR *_text)
{
  text = _text;
  Invalidate();
}

unsigned
WndFrame::GetTextHeight() const
{
  PixelRect rc = GetClientRect();
  const int padding = Layout::GetTextPadding();
  rc.Grow(-padding);

  AnyCanvas canvas;
  canvas.Select(*look.text_font);
  canvas.DrawFormattedText(&rc, text.c_str(), mCaptionStyle | DT_CALCRECT);

  return rc.bottom - rc.top;
}

void
WndFrame::OnPaint(Canvas &canvas)
{
#ifdef HAVE_CLIPPING
  canvas.Clear(look.background_brush);
#endif

  canvas.SetTextColor(caption_color);
  canvas.SetBackgroundTransparent();

  canvas.Select(*look.text_font);

  PixelRect rc = GetClientRect();
  const int padding = Layout::GetTextPadding();
  rc.Grow(-padding);

  canvas.DrawFormattedText(&rc, text.c_str(), mCaptionStyle);
}
