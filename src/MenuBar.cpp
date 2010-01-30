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

#include "MenuBar.hpp"
#include "Screen/ContainerWindow.hpp"
#include "Screen/Layout.hpp"
#include "Screen/Animation.hpp"

#include <assert.h>

void
GetButtonPosition(unsigned i, RECT rc, int *x, int *y, int *sizex, int *sizey)
{
  int hwidth = (rc.right - rc.left) / 4;
  int hheight = (rc.bottom - rc.top) / 4;

  if (hheight > hwidth) {
    // portrait

    if (i == 0) {
      *sizex = IBLSCALE(52);
      *sizey = IBLSCALE(37);
      *x = rc.left - (*sizex); // JMW make it offscreen for now
      *y = rc.bottom - (*sizey);
    } else if (i < 5) {
      *sizex = IBLSCALE(52);
      *sizey = IBLSCALE(40);
      *x = rc.left + 3 + hwidth * (i - 1);
      *y = rc.bottom - (*sizey);
    } else {
      *sizex = IBLSCALE(80);
      *sizey = IBLSCALE(40);
      *x = rc.right - (*sizex);
      int k = rc.bottom - rc.top - IBLSCALE(46);

      if (is_altair()) {
        k = rc.bottom - rc.top;
        // JMW need upside down button order for rotated Altair
        *y = rc.bottom - (i - 5) * k / 5 - (*sizey) - IBLSCALE(20);
      } else {
        *y = rc.top + (i - 5) * k / 6 + (*sizey / 2 + IBLSCALE(3));
      }
    }
  } else {
    // landscape

    hwidth = (rc.right - rc.left) / 5;
    hheight = (rc.bottom - rc.top) / 5;

    if (i == 0) {
      *sizex = IBLSCALE(52);
      *sizey = IBLSCALE(20);
      *x = rc.left - (*sizex); // JMW make it offscreen for now
      *y = (rc.top);
    } else if (i < 5) {
      *sizex = IBLSCALE(52);
      *sizey = is_altair() ? IBLSCALE(20) : IBLSCALE(35);
      *x = rc.left + 3;
      *y = (rc.top + hheight * i - (*sizey) / 2);
    } else {
      *sizex = IBLSCALE(60);
      *sizey = is_altair() ? IBLSCALE(40) : IBLSCALE(35);
      *x = rc.left + hwidth * (i - 5);
      *y = (rc.bottom - (*sizey));
    }
  }
}

MenuBar::MenuBar(ContainerWindow &parent)
{
  const RECT rc = parent.get_client_rect();
  int x, y, xsize, ysize;

  for (unsigned i = 0; i < MAX_BUTTONS; ++i) {
    GetButtonPosition(i, rc, &x, &y, &xsize, &ysize);
    buttons[i].set(parent, _T(""), FIRST_ID + i, x, y, xsize, ysize,
                   false, false, true);
  }
}

void
MenuBar::SetFont(const Font &font)
{
  for (unsigned i = 0; i < MAX_BUTTONS; i++)
    buttons[i].set_font(font);
}

void
MenuBar::ShowButton(unsigned i, bool enabled, const TCHAR *text)
{
  assert(i < MAX_BUTTONS);

  ButtonWindow &button = buttons[i];

  button.set_text(text);
  button.set_enabled(enabled);
  button.show_on_top();
}

void
MenuBar::HideButton(unsigned i)
{
  assert(i < MAX_BUTTONS);

  buttons[i].hide();
}

void
MenuBar::AnimateButton(unsigned i)
{
  assert(i < MAX_BUTTONS);

  if (!buttons[i].is_visible())
    return;

  RECT mRc, aniRect;
  mRc = buttons[i].get_screen_position();

  aniRect.top = (mRc.top * 5 + mRc.bottom) / 6;
  aniRect.left = (mRc.left * 5 + mRc.right) / 6;
  aniRect.right = (mRc.left + mRc.right * 5) / 6;
  aniRect.bottom = (mRc.top + mRc.bottom * 5) / 6;
  SetSourceRectangle(aniRect);
  DrawWireRects(true, &mRc, 5);

  SetSourceRectangle(mRc);
}
