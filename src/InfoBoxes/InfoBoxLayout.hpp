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

#ifndef XCSOAR_INFO_BOX_LAYOUT_HPP
#define XCSOAR_INFO_BOX_LAYOUT_HPP

#include <windef.h>

namespace InfoBoxLayout
{
  enum Layouts {
    // 0: default, infoboxes along top and bottom, map in middle
    ibTop4Bottom4 = 0,
    // 1: both infoboxes along bottom
    ibBottom8 = 1,
    // 2: both infoboxes along top
    ibTop8 = 2,
    // 3: infoboxes along both sides
    ibLeft4Right4 = 3,
    // 4: infoboxes along left side
    ibLeft8 = 4,
    // 5: infoboxes along right side
    ibRight8 = 5,
    // 6: infoboxes GNAV
    ibGNav = 6,
    // 7: infoboxes (5) along right side (square screen)
    ibSquare = 7
  };

  extern bool fullscreen;
  extern Layouts InfoBoxGeometry;
  extern unsigned ControlWidth, ControlHeight, TitleHeight;
  extern unsigned numInfoWindows;

  void Init(RECT rc);
  void LoadGeometry(RECT rc);

  // used by manager
  void GetInfoBoxPosition(unsigned i, RECT rc, int *x, int *y,
                                 int *sizex, int *sizey);

  void CalcInfoBoxSizes(RECT rc);
  RECT GetRemainingRect(RECT rc);
};

#endif
