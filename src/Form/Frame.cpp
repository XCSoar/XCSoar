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

#include "Form/Frame.hpp"
#include "Screen/VirtualCanvas.hpp"

WndFrame::WndFrame(ContainerControl &parent,
                   int X, int Y, int Width, int Height,
                   const WindowStyle style)
  :mCaptionStyle(DT_EXPANDTABS | DT_LEFT | DT_NOCLIP | DT_WORDBREAK)
{
  SetForeColor(parent.GetForeColor());
  SetBackColor(parent.GetBackColor());

  set(parent.GetClientAreaWindow(), X, Y, Width, Height, style);
}

void
WndFrame::SetAlignCenter()
{
  mCaptionStyle &= ~(DT_LEFT|DT_RIGHT);
  mCaptionStyle |= DT_CENTER;
  invalidate();
}

unsigned
WndFrame::GetTextHeight()
{
  RECT rc = get_client_rect();
  ::InflateRect(&rc, -2, -2); // todo border width

  VirtualCanvas canvas(1, 1);
  canvas.select(*GetFont());
  canvas.formatted_text(&rc, mCaption, mCaptionStyle | DT_CALCRECT);

  return rc.bottom - rc.top;
}

void
WndFrame::on_paint(Canvas &canvas)
{
  canvas.clear(GetBackColor());

  if (mCaption[0] == 0)
    return;

  canvas.set_text_color(GetForeColor());
  canvas.set_background_color(GetBackColor());
  canvas.background_transparent();

  canvas.select(*GetFont());

  RECT rc = get_client_rect();
  InflateRect(&rc, -2, -2); // todo border width

  canvas.formatted_text(&rc, mCaption, mCaptionStyle);
}
