/*
Copyright_License {

  XCSoar Glide Computer - http://www.xcsoar.org/
  Copyright (C) 2000 - 2009

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

#include "InfoBoxLayout.h"
#include "InfoBoxManager.h"
#include "ButtonLabel.h"
#include "LogFile.hpp"
#include "SettingsUser.hpp"
#include "Screen/Animation.hpp"
#include "Registry.hpp"
#include "InfoBox.h"
#include "WindowControls.h"
#include <stdio.h>

// Layouts:
// 0: default, infoboxes along top and bottom, map in middle
// 1: both infoboxes along bottom
// 2: both infoboxes along top
// 3: infoboxes along both sides
// 4: infoboxes along left side
// 5: infoboxes along right side
// 6: infoboxes GNAV
/*

Screen
640x480 landscape

480/6 = 80 control height

2/3 of width is map = 420
leaving 220 = 110 control width

*/


/*

Button 0 (x,y,sx,sy)
Button 1 (x,y,sx,sy)
...

InfoBox 0 (x,y,sx,sy)

*/


unsigned InfoBoxLayout::InfoBoxGeometry = 0;
int InfoBoxLayout::ControlWidth;
int InfoBoxLayout::ControlHeight;
int InfoBoxLayout::TitleHeight;
int InfoBoxLayout::scale = 1;
double InfoBoxLayout::dscale=1.0;
bool InfoBoxLayout::IntScaleFlag=false;

bool InfoBoxLayout::gnav = false;

bool geometrychanged = false;

bool InfoBoxLayout::landscape = false;
bool InfoBoxLayout::square = false;
bool InfoBoxLayout::fullscreen = false;

void InfoBoxLayout::GetInfoBoxPosition(unsigned i, RECT rc,
				       int *x, int *y,
                                       int *sizex, int *sizey) {
  TCHAR reggeompx[50];
  TCHAR reggeompy[50];
  TCHAR reggeomsx[50];
  TCHAR reggeomsy[50];
  DWORD Temp=0;

  _stprintf(reggeompx, TEXT("InfoBoxPositionPosX%u"), i);
  _stprintf(reggeompy, TEXT("InfoBoxPositionPosY%u"), i);
  _stprintf(reggeomsx, TEXT("InfoBoxPositionSizeX%u"), i);
  _stprintf(reggeomsy, TEXT("InfoBoxPositionSizeY%u"), i);

  GetFromRegistry(reggeompx,&Temp); *x = Temp;
  GetFromRegistry(reggeompy,&Temp); *y = Temp;
  GetFromRegistry(reggeomsx,&Temp); *sizex = Temp;
  GetFromRegistry(reggeomsy,&Temp); *sizey = Temp;

  if (*sizey != ControlHeight) {
    geometrychanged = true;
  }
  if (*sizex != ControlWidth) {
    geometrychanged = true;
  }

  if ((*sizex==0)||(*sizey==0)||geometrychanged) {
    // not defined in registry so go with defaults
    // these will be saved back to registry

    switch (InfoBoxGeometry) {
    case 0:
      if (i<numInfoWindows/2) {
	*x = i*ControlWidth;
	*y = rc.top;
      } else {
	*x = (i-numInfoWindows/2)*ControlWidth;
	*y = rc.bottom-ControlHeight;
      }
      break;
    case 1:
      if (i<numInfoWindows/2) {
	*x = i*ControlWidth;
	*y = rc.bottom-ControlHeight*2;
      } else {
	*x = (i-numInfoWindows/2)*ControlWidth;
	*y = rc.bottom-ControlHeight;
      }
      break;
    case 2:
      if (i<numInfoWindows/2) {
	*x = i*ControlWidth;
	*y = rc.top;;
      } else {
	*x = (i-numInfoWindows/2)*ControlWidth;
	*y = rc.top+ControlHeight;
      }
      break;

    case 3:
      if (i<numInfoWindows/2) {
	*x = rc.left;
	*y = rc.top+ControlHeight*i;
      } else {
	*x = rc.right-ControlWidth;
	*y = rc.top+ControlHeight*(i-numInfoWindows/2);
      }
      break;
    case 4:
      if (i<numInfoWindows/2) {
	*x = rc.left;
	*y = rc.top+ControlHeight*i;
      } else {
	*x = rc.left+ControlWidth;
	*y = rc.top+ControlHeight*(i-numInfoWindows/2);
      }
      break;
    case 5:
      if (i<numInfoWindows/2) {
	*x = rc.right-ControlWidth*2;
	*y = rc.top+ControlHeight*i;
      } else {
	*x = rc.right-ControlWidth;
	*y = rc.top+ControlHeight*(i-numInfoWindows/2);
      }
      break;
    case 6:
      if (i<3) {
	*x = rc.right-ControlWidth*2;
	*y = rc.top+ControlHeight*i;
      } else {
	if (i<6) {
	  *x = rc.right-ControlWidth*2;
	  *y = rc.top+ControlHeight*(i-3)+ControlHeight*3;
	} else {
	  *x = rc.right-ControlWidth;
	  *y = rc.top+ControlHeight*(i-6)+ControlHeight*3;
	}
      }
      break;
    case 7:
      *x = rc.right-ControlWidth;
      *y = rc.top+ControlHeight*i;
      break;
    };

    *sizex = ControlWidth;
    *sizey = ControlHeight;

    SetToRegistry(reggeompx,*x);
    SetToRegistry(reggeompy,*y);
    SetToRegistry(reggeomsx, (int &)*sizex);
    SetToRegistry(reggeomsy, (int &)*sizey);

  };
}


//
// Paolo Ventafridda, VENTA-ADDON Geometry change in Config menu 11
//
void InfoBoxLayout::ScreenGeometry(RECT rc) {

  TCHAR szRegistryInfoBoxGeometry[]=  TEXT("InfoBoxGeometry");

  DWORD Temp=0;
  GetFromRegistry(szRegistryInfoBoxGeometry,&Temp);
  InfoBoxGeometry = Temp;

#if defined(PNA) || defined(FIVV)
// VENTA-ADDON GEOM
  static const TCHAR szRegistryInfoBoxGeom[]=  TEXT("AppInfoBoxGeom");
  GetFromRegistry(szRegistryInfoBoxGeom,&Temp);
  if (InfoBoxGeometry != Temp) {
    StartupStore(_T("Geometry was changed in config, applying\n"));
    InfoBoxGeometry=Temp;
  }
#endif

  // JMW testing only
  geometrychanged = true;

  int maxsize=0;
  int minsize=0;
  maxsize = max(rc.right-rc.left,rc.bottom-rc.top);
  minsize = min(rc.right-rc.left,rc.bottom-rc.top);

  dscale = max(1,minsize/240.0); // always start w/ shortest dimension

  if (maxsize == minsize)  // square should be shrunk
  {
    dscale *= 240.0 / 320.0;
  }

  scale = (int)dscale;
  if ( ((double)scale) == dscale)
    IntScaleFlag=true;
  else
    IntScaleFlag=false;

  if (rc.bottom<rc.right) {
    // landscape mode
    landscape = true;
    if (InfoBoxGeometry<4) {
      geometrychanged = true;

      // JMW testing
      if (1) {
	InfoBoxGeometry = 6;
      } else {
	InfoBoxGeometry+= 3;
      }
    }

  } else if (rc.bottom==rc.right) {
    landscape = false;
    square = true;
    if (InfoBoxGeometry<7) {
      geometrychanged = true;
    }
    InfoBoxGeometry = 7;

  } else {
    landscape = false;
    // portrait mode
    gnav = false;
    if (InfoBoxGeometry>=3) {
      InfoBoxGeometry= 0;

      geometrychanged = true;
    }
  }

  SetToRegistry(szRegistryInfoBoxGeometry, (int &)InfoBoxGeometry);

  // JMW testing
  if (InfoBoxGeometry==6) {
    gnav = true;
  }

  if (gnav) {
    numInfoWindows = 9;
  } else if (square) {
    numInfoWindows = 5;
  } else {
    numInfoWindows = 8;
  }

//
// VENTA3 disable gauge vario for geometry 5 in landscape mode, use 8 box right instead
// beside those boxes were painted and overwritten by the gauge already and gauge was
// graphically too much stretched, requiring a restyle!
  if (gnav) {
      if ( ( landscape == true) && (InfoBoxGeometry == 5 ) )
      	EnableVarioGauge = false;
      else
      	EnableVarioGauge = true;
  } else {
    EnableVarioGauge = false;
  }

}


RECT InfoBoxLayout::GetInfoBoxSizes(RECT rc) {

  RECT MapRect;

  switch (InfoBoxGeometry) {
  case 0: // portrait
    // calculate control dimensions
    ControlWidth = 2*(rc.right - rc.left) / numInfoWindows;
    ControlHeight = (unsigned)((rc.bottom - rc.top) / CONTROLHEIGHTRATIO);
    TitleHeight = (unsigned)(ControlHeight/TITLEHEIGHTRATIO);

    // calculate small map screen size

    MapRect.top = rc.top+ControlHeight;
    MapRect.left = rc.left;
    MapRect.bottom = rc.bottom-ControlHeight;
    MapRect.right = rc.right;
    break;

  case 1: // not used
    // calculate control dimensions

    ControlWidth = 2*(rc.right - rc.left) / numInfoWindows;
    ControlHeight = (unsigned)((rc.bottom - rc.top) / CONTROLHEIGHTRATIO);
    TitleHeight = (unsigned)(ControlHeight/TITLEHEIGHTRATIO);

    // calculate small map screen size

    MapRect.top = rc.top;
    MapRect.left = rc.left;
    MapRect.bottom = rc.bottom-ControlHeight*2;
    MapRect.right = rc.right;
    break;

  case 2: // not used
    // calculate control dimensions

    ControlWidth = 2*(rc.right - rc.left) / numInfoWindows;
    ControlHeight = (unsigned)((rc.bottom - rc.top) / CONTROLHEIGHTRATIO);
    TitleHeight = (unsigned)(ControlHeight/TITLEHEIGHTRATIO);

    // calculate small map screen size

    MapRect.top = rc.top+ControlHeight*2;
    MapRect.left = rc.left;
    MapRect.bottom = rc.bottom;
    MapRect.right = rc.right;
    break;

  case 3: // not used
    // calculate control dimensions

    ControlWidth = (unsigned)((rc.right - rc.left) / CONTROLHEIGHTRATIO*1.3);
    ControlHeight = (unsigned)(2*(rc.bottom - rc.top) / numInfoWindows);
    TitleHeight = (unsigned)(ControlHeight/TITLEHEIGHTRATIO);

    // calculate small map screen size

    MapRect.top = rc.top;
    MapRect.left = rc.left+ControlWidth;
    MapRect.bottom = rc.bottom;
    MapRect.right = rc.right-ControlWidth;
    break;

  case 4:
    // calculate control dimensions

    ControlWidth = (unsigned)((rc.right - rc.left) / CONTROLHEIGHTRATIO*1.3);
    ControlHeight = (unsigned)(2*(rc.bottom - rc.top) / numInfoWindows);
    TitleHeight = (unsigned)(ControlHeight/TITLEHEIGHTRATIO);

    // calculate small map screen size

    MapRect.top = rc.top;
    MapRect.left = rc.left+ControlWidth*2;
    MapRect.bottom = rc.bottom;
    MapRect.right = rc.right;
    break;

  case 5: // not used
    // calculate control dimensions

    ControlWidth = (unsigned)((rc.right - rc.left) / CONTROLHEIGHTRATIO*1.3);
    ControlHeight = (unsigned)(2*(rc.bottom - rc.top) / numInfoWindows);
    TitleHeight = (unsigned)(ControlHeight/TITLEHEIGHTRATIO);

    // calculate small map screen size

    MapRect.top = rc.top;
    MapRect.left = rc.left;
    MapRect.bottom = rc.bottom;
    MapRect.right = rc.right-ControlWidth*2;
    break;

  case 6: // landscape
    // calculate control dimensions

    ControlHeight = (unsigned)((rc.bottom - rc.top)/6);
    ControlWidth=(unsigned)(ControlHeight*1.44); // preserve relative shape
    TitleHeight = (unsigned)(ControlHeight/TITLEHEIGHTRATIO);

    // calculate small map screen size

    MapRect.top = rc.top;
    MapRect.left = rc.left;
    MapRect.bottom = rc.bottom;
    MapRect.right = rc.right-ControlWidth*2;

    break;

  case 7: // square
    // calculate control dimensions

    ControlWidth = (unsigned)((rc.right - rc.left)*0.2);
    ControlHeight = (unsigned)((rc.bottom - rc.top)/5);
    TitleHeight = (unsigned)(ControlHeight/TITLEHEIGHTRATIO);

    // calculate small map screen size

    MapRect.top = rc.top;
    MapRect.left = rc.left;
    MapRect.bottom = rc.bottom;
    MapRect.right = rc.right-ControlWidth;

    break;
  };
  return MapRect;
}

