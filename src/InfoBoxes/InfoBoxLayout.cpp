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

#include "InfoBoxes/InfoBoxLayout.hpp"
#include "InfoBoxes/InfoBoxManager.hpp"
#include "InfoBoxes/InfoBoxWindow.hpp"
#include "LogFile.hpp"
#include "SettingsMap.hpp"
#include "Screen/Layout.hpp"
#include "Profile/Profile.hpp"
#include "Sizes.h"

#include <stdio.h>


/*
Screen
640x480 landscape

480/6 = 80 control height

2/3 of width is map = 420
leaving 220 = 110 control width
*/

namespace InfoBoxLayout
{
  Layouts InfoBoxGeometry = ibTop4Bottom4;
  unsigned ControlWidth;
  unsigned ControlHeight;
  unsigned TitleHeight;
  bool fullscreen = false;
  unsigned numInfoWindows = 8;
}

void
InfoBoxLayout::Init(RECT rc)
{
  LoadGeometry();
  CalcInfoBoxSizes(rc);
}

void
InfoBoxLayout::GetInfoBoxPosition(unsigned i, RECT rc, int *x, int *y,
    int *sizex, int *sizey)
{
  switch (InfoBoxGeometry) {
  case ibTop4Bottom4:
    if (i < numInfoWindows / 2) {
      *x = i * ControlWidth;
      *y = rc.top;
    } else {
      *x = (i - numInfoWindows / 2) * ControlWidth;
      *y = rc.bottom - ControlHeight;
    }
    break;

  case ibBottom8:
    if (i < numInfoWindows / 2) {
      *x = i * ControlWidth;
      *y = rc.bottom - ControlHeight * 2;
    } else {
      *x = (i - numInfoWindows / 2) * ControlWidth;
      *y = rc.bottom - ControlHeight;
    }
    break;

  case ibTop8:
    if (i < numInfoWindows / 2) {
      *x = i * ControlWidth;
      *y = rc.top;
    } else {
      *x = (i - numInfoWindows / 2) * ControlWidth;
      *y = rc.top + ControlHeight;
    }
    break;

  case ibLeft4Right4:
    if (i < numInfoWindows / 2) {
      *x = rc.left;
      *y = rc.top + ControlHeight * i;
    } else {
      *x = rc.right - ControlWidth;
      *y = rc.top + ControlHeight * (i - numInfoWindows / 2);
    }
    break;

  case ibLeft8:
    if (i < numInfoWindows / 2) {
      *x = rc.left;
      *y = rc.top + ControlHeight * i;
    } else {
      *x = rc.left + ControlWidth;
      *y = rc.top + ControlHeight * (i - numInfoWindows / 2);
    }
    break;

  case ibRight8:
  case ibRight12:
    if (i < numInfoWindows / 2) {
      *x = rc.right - ControlWidth * 2;
      *y = rc.top + ControlHeight * i;
    } else {
      *x = rc.right - ControlWidth;
      *y = rc.top + ControlHeight * (i - numInfoWindows / 2);
    }
    break;

  case ibGNav:
    if (i < 3) {
      *x = rc.right - ControlWidth * 2;
      *y = rc.top + ControlHeight * i;
    } else if (i < 6) {
      *x = rc.right - ControlWidth * 2;
      *y = rc.top + ControlHeight * (i - 3) + ControlHeight * 3;
    } else {
      *x = rc.right - ControlWidth;
      *y = rc.top + ControlHeight * (i - 6) + ControlHeight * 3;
    }
    break;

  case ibSquare:
    *x = rc.right - ControlWidth;
    *y = rc.top + ControlHeight * i;
    break;
  };

  *sizex = ControlWidth;
  *sizey = ControlHeight;
}

void
InfoBoxLayout::LoadGeometry()
{
  unsigned tmp;
  if (Profile::Get(szProfileInfoBoxGeometry, tmp))
    InfoBoxGeometry = (Layouts)tmp;

  if (Layout::landscape) {
    if (InfoBoxGeometry < ibLeft4Right4)
      InfoBoxGeometry = ibGNav;
  } else if (Layout::square) {
    InfoBoxGeometry = ibSquare;
  } else {
    if (InfoBoxGeometry >= ibLeft4Right4)
      InfoBoxGeometry = ibTop4Bottom4;
  }

  Profile::Set(szProfileInfoBoxGeometry, (unsigned)InfoBoxGeometry);

  if (InfoBoxGeometry == ibGNav)
    numInfoWindows = 9;
  else if (InfoBoxGeometry == ibRight12)
    numInfoWindows = 12;
  else if (InfoBoxGeometry == ibSquare)
    numInfoWindows = 5;
  else
    numInfoWindows = 8;

  assert(numInfoWindows <= InfoBoxPanelConfig::MAX_INFOBOXES);
}

void
InfoBoxLayout::CalcInfoBoxSizes(RECT rc)
{
  switch (InfoBoxGeometry) {
  case ibTop4Bottom4:
  case ibBottom8:
  case ibTop8:
    // calculate control dimensions
    ControlWidth = 2 * (rc.right - rc.left) / numInfoWindows;
    ControlHeight = (rc.bottom - rc.top) / CONTROLHEIGHTRATIO;
    break;

  case ibLeft4Right4:
  case ibLeft8:
  case ibRight8:
    // calculate control dimensions
    ControlWidth = (rc.right - rc.left) / CONTROLHEIGHTRATIO * 1.3;
    ControlHeight = 2 * (rc.bottom - rc.top) / numInfoWindows;
    break;

  case ibGNav:
  case ibRight12:
    // calculate control dimensions
    ControlHeight = (rc.bottom - rc.top) / 6;
    ControlWidth = ControlHeight * 1.44; // preserve relative shape
    break;

  case ibSquare:
    // calculate control dimensions
    ControlWidth = (rc.right - rc.left) * 0.2;
    ControlHeight = (rc.bottom - rc.top) / 5;
    break;
  }

  TitleHeight = ControlHeight / 3.1;
}

RECT
InfoBoxLayout::GetRemainingRect(RECT rc)
{
  RECT MapRect = rc;

  switch (InfoBoxGeometry) {
  case ibTop4Bottom4:
    // calculate small map screen size
    MapRect.top = rc.top + ControlHeight;
    MapRect.bottom = rc.bottom - ControlHeight;
    break;

  case ibBottom8:
    // calculate small map screen size
    MapRect.bottom = rc.bottom - ControlHeight * 2;
    break;

  case ibTop8:
    // calculate small map screen size
    MapRect.top = rc.top + ControlHeight * 2;
    break;

  case ibLeft4Right4:
    // calculate small map screen size
    MapRect.left = rc.left + ControlWidth;
    MapRect.right = rc.right - ControlWidth;
    break;

  case ibLeft8:
    // calculate small map screen size
    MapRect.left = rc.left + ControlWidth * 2;
    break;

  case ibRight8:
    // calculate small map screen size
    MapRect.right = rc.right - ControlWidth * 2;
    break;

  case ibGNav:
  case ibRight12:
    // calculate small map screen size
    MapRect.right = rc.right - ControlWidth * 2;
    break;

  case ibSquare:
    // calculate small map screen size
    MapRect.right = rc.right - ControlWidth;
    break;
  }

  return MapRect;
}
