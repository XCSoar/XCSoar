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

#include "Form/SymbolButton.hpp"
#include "Form/Container.hpp"
#include "Screen/Animation.hpp"
#include "Screen/Layout.hpp"
#include "Interface.hpp"

void
WndSymbolButton::on_paint(Canvas &canvas)
{
  // Call parent on_paint function (selector/focus)
  WindowControl::on_paint(canvas);

  // Get button RECT and shrink it to make room for the selector/focus
  RECT rc = get_client_rect();
  InflateRect(&rc, -2, -2); // todo border width

  // Draw button to the background
  canvas.draw_button(rc, mDown);

  // If button has text on it
  if (mCaption == NULL || mCaption[0] == '\0')
    return;

  // Set drawing colors
  canvas.set_background_color(GetBackColor());
  canvas.background_transparent();

  // Set drawing font
  canvas.select(*GetFont());

  // If button is pressed, offset the text for 3D effect
  if (mDown)
    OffsetRect(&rc, Layout::FastScale(1), Layout::FastScale(1));

  static Pen p(0, GetForeColor());
  canvas.select(p);
  static Brush b(GetForeColor());
  canvas.select(b);

  // Draw arrow symbols instead of < and >
  if (!_tcscmp(mCaption, _T("<")) || !_tcscmp(mCaption, _T(">"))) {
    int size = min(rc.right - rc.left, rc.bottom - rc.top) / 5;

    static POINT Arrow[4];
    Arrow[0].x = (rc.left + rc.right) / 2 +
                 (!_tcscmp(mCaption, _T("<")) ? size : -size);
    Arrow[0].y = (rc.top + rc.bottom) / 2 + size;
    Arrow[1].x = (rc.left + rc.right) / 2 +
                 (!_tcscmp(mCaption, _T("<")) ? -size : size);
    Arrow[1].y = (rc.top + rc.bottom) / 2;
    Arrow[2].x = (rc.left + rc.right) / 2 +
                 (!_tcscmp(mCaption, _T("<")) ? size : -size);
    Arrow[2].y = (rc.top + rc.bottom) / 2 - size;
    Arrow[3].x = Arrow[0].x;
    Arrow[3].y = Arrow[0].y;

    canvas.polygon(Arrow, 4);
  }

  // Draw symbols instead of + and -
  if (!_tcscmp(mCaption, _T("+")) || !_tcscmp(mCaption, _T("-"))) {
    int size = min(rc.right - rc.left, rc.bottom - rc.top) / 5;

    static POINT Arrow[5];
    Arrow[0].x = (rc.left + rc.right) / 2 - size;
    Arrow[0].y = (rc.top + rc.bottom) / 2 - size / 3;
    Arrow[1].x = (rc.left + rc.right) / 2 - size;
    Arrow[1].y = (rc.top + rc.bottom) / 2 + size / 3;
    Arrow[2].x = (rc.left + rc.right) / 2 + size;
    Arrow[2].y = (rc.top + rc.bottom) / 2 + size / 3;
    Arrow[3].x = (rc.left + rc.right) / 2 + size;
    Arrow[3].y = (rc.top + rc.bottom) / 2 - size / 3;
    Arrow[4].x = Arrow[0].x;
    Arrow[4].y = Arrow[0].y;

    canvas.polygon(Arrow, 5);

    if (!_tcscmp(mCaption, _T("+"))) {
      Arrow[0].x = (rc.left + rc.right) / 2 - size / 3;
      Arrow[0].y = (rc.top + rc.bottom) / 2 - size;
      Arrow[1].x = (rc.left + rc.right) / 2 - size / 3;
      Arrow[1].y = (rc.top + rc.bottom) / 2 + size;
      Arrow[2].x = (rc.left + rc.right) / 2 + size / 3;
      Arrow[2].y = (rc.top + rc.bottom) / 2 + size;
      Arrow[3].x = (rc.left + rc.right) / 2 + size / 3;
      Arrow[3].y = (rc.top + rc.bottom) / 2 - size;
      Arrow[4].x = Arrow[0].x;
      Arrow[4].y = Arrow[0].y;

      canvas.polygon(Arrow, 5);
    }
  }
}
