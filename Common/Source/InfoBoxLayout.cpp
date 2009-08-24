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
#include "XCSoar.h"
#include "MapWindow.h"
#include "Dialogs.h"
#include "Screen/Animation.hpp"
#include "Registry.hpp"
#include "InfoBox.h"
#include "WindowControls.h"
#include "ExpandMacros.hpp"
#include "Interface.hpp"

extern InfoBox *InfoBoxes[MAXINFOWINDOWS];

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


int InfoBoxLayout::InfoBoxGeometry = 0;
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

void InfoBoxLayout::GetInfoBoxPosition(int i, RECT rc,
				       int *x, int *y,
				       int *sizex, int *sizey) {
  TCHAR reggeompx[50];
  TCHAR reggeompy[50];
  TCHAR reggeomsx[50];
  TCHAR reggeomsy[50];
  DWORD Temp=0;

  _stprintf(reggeompx, TEXT("InfoBoxPositionPosX%d"), i);
  _stprintf(reggeompy, TEXT("InfoBoxPositionPosY%d"), i);
  _stprintf(reggeomsx, TEXT("InfoBoxPositionSizeX%d"), i);
  _stprintf(reggeomsy, TEXT("InfoBoxPositionSizeY%d"), i);

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
    SetToRegistry(reggeomsx,*sizex);
    SetToRegistry(reggeomsy,*sizey);

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

  SetToRegistry(szRegistryInfoBoxGeometry,InfoBoxGeometry);

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
}


void InfoBoxLayout::GetInfoBoxSizes(RECT rc) {

  switch (InfoBoxGeometry) {
  case 0: // portrait
    // calculate control dimensions
    ControlWidth = 2*(rc.right - rc.left) / numInfoWindows;
    ControlHeight = (int)((rc.bottom - rc.top) / CONTROLHEIGHTRATIO);
    TitleHeight = (int)(ControlHeight/TITLEHEIGHTRATIO);

    // calculate small map screen size

    MapWindow::MapRect.top = rc.top+ControlHeight;
    MapWindow::MapRect.left = rc.left;
    MapWindow::MapRect.bottom = rc.bottom-ControlHeight;
    MapWindow::MapRect.right = rc.right;
    break;

  case 1: // not used
    // calculate control dimensions

    ControlWidth = 2*(rc.right - rc.left) / numInfoWindows;
    ControlHeight = (int)((rc.bottom - rc.top) / CONTROLHEIGHTRATIO);
    TitleHeight = (int)(ControlHeight/TITLEHEIGHTRATIO);

    // calculate small map screen size

    MapWindow::MapRect.top = rc.top;
    MapWindow::MapRect.left = rc.left;
    MapWindow::MapRect.bottom = rc.bottom-ControlHeight*2;
    MapWindow::MapRect.right = rc.right;
    break;

  case 2: // not used
    // calculate control dimensions

    ControlWidth = 2*(rc.right - rc.left) / numInfoWindows;
    ControlHeight = (int)((rc.bottom - rc.top) / CONTROLHEIGHTRATIO);
    TitleHeight = (int)(ControlHeight/TITLEHEIGHTRATIO);

    // calculate small map screen size

    MapWindow::MapRect.top = rc.top+ControlHeight*2;
    MapWindow::MapRect.left = rc.left;
    MapWindow::MapRect.bottom = rc.bottom;
    MapWindow::MapRect.right = rc.right;
    break;

  case 3: // not used
    // calculate control dimensions

    ControlWidth = (int)((rc.right - rc.left) / CONTROLHEIGHTRATIO*1.3);
    ControlHeight = (int)(2*(rc.bottom - rc.top) / numInfoWindows);
    TitleHeight = (int)(ControlHeight/TITLEHEIGHTRATIO);

    // calculate small map screen size

    MapWindow::MapRect.top = rc.top;
    MapWindow::MapRect.left = rc.left+ControlWidth;
    MapWindow::MapRect.bottom = rc.bottom;
    MapWindow::MapRect.right = rc.right-ControlWidth;
    break;

  case 4:
    // calculate control dimensions

    ControlWidth = (int)((rc.right - rc.left) / CONTROLHEIGHTRATIO*1.3);
    ControlHeight = (int)(2*(rc.bottom - rc.top) / numInfoWindows);
    TitleHeight = (int)(ControlHeight/TITLEHEIGHTRATIO);

    // calculate small map screen size

    MapWindow::MapRect.top = rc.top;
    MapWindow::MapRect.left = rc.left+ControlWidth*2;
    MapWindow::MapRect.bottom = rc.bottom;
    MapWindow::MapRect.right = rc.right;
    break;

  case 5: // not used
    // calculate control dimensions

    ControlWidth = (int)((rc.right - rc.left) / CONTROLHEIGHTRATIO*1.3);
    ControlHeight = (int)(2*(rc.bottom - rc.top) / numInfoWindows);
    TitleHeight = (int)(ControlHeight/TITLEHEIGHTRATIO);

    // calculate small map screen size

    MapWindow::MapRect.top = rc.top;
    MapWindow::MapRect.left = rc.left;
    MapWindow::MapRect.bottom = rc.bottom;
    MapWindow::MapRect.right = rc.right-ControlWidth*2;
    break;

  case 6: // landscape
    // calculate control dimensions

    ControlHeight = (int)((rc.bottom - rc.top)/6);
    ControlWidth=(int)(ControlHeight*1.44); // preserve relative shape
    TitleHeight = (int)(ControlHeight/TITLEHEIGHTRATIO);

    // calculate small map screen size

    MapWindow::MapRect.top = rc.top;
    MapWindow::MapRect.left = rc.left;
    MapWindow::MapRect.bottom = rc.bottom;
    MapWindow::MapRect.right = rc.right-ControlWidth*2;

    break;

  case 7: // square
    // calculate control dimensions

    ControlWidth = (int)((rc.right - rc.left)*0.2);
    ControlHeight = (int)((rc.bottom - rc.top)/5);
    TitleHeight = (int)(ControlHeight/TITLEHEIGHTRATIO);

    // calculate small map screen size

    MapWindow::MapRect.top = rc.top;
    MapWindow::MapRect.left = rc.left;
    MapWindow::MapRect.bottom = rc.bottom;
    MapWindow::MapRect.right = rc.right-ControlWidth;

    break;
  };

}


void InfoBoxLayout::Paint(void) {
  int i;
  for (i=0; i<numInfoWindows; i++)
    InfoBoxes[i]->Paint();

  if (!fullscreen) {
    InfoBoxes[numInfoWindows]->SetVisible(false);
    for (i=0; i<numInfoWindows; i++)
      InfoBoxes[i]->PaintFast();
  } else {
    InfoBoxes[numInfoWindows]->SetVisible(true);
    for (i=0; i<numInfoWindows; i++) {
      int x, y;
      int rx, ry;
      int rw;
      int rh;
      double fw, fh;
      if (landscape) {
        rw = 84;
        rh = 68;
      } else {
        rw = 120;
        rh = 80;
      }
      fw = rw/(double)ControlWidth;
      fh = rh/(double)ControlHeight;
      double f = min(fw, fh);
      rw = (int)(f*ControlWidth);
      rh = (int)(f*ControlHeight);

      if (landscape) {
        rx = i % 3;
        ry = i / 3;

        x = (rw+4)*rx;
        y = (rh+3)*ry;

      } else {
        rx = i % 2;
        ry = i / 4;

        x = (rw)*rx;
        y = (rh)*ry;

      }
      InfoBoxes[i]->PaintInto(InfoBoxes[numInfoWindows]->GetHdcBuf(),
                              IBLSCALE(x), IBLSCALE(y), IBLSCALE(rw), IBLSCALE(rh));
    }
    InfoBoxes[numInfoWindows]->PaintFast();
  }
}


void InfoBoxLayout::CreateInfoBoxes(RECT rc) {
  int i;
  int xoff, yoff, sizex, sizey;

  GetInfoBoxSizes(rc);

  // JMW created full screen infobox mode
  xoff=0;
  yoff=0;
  sizex=rc.right-rc.left;
  sizey=rc.bottom-rc.top;

  InfoBoxes[numInfoWindows] = new InfoBox(hWndMainWindow, xoff, yoff, sizex, sizey);
  InfoBoxes[numInfoWindows]->SetBorderKind(0);

  // create infobox windows

  for(i=0;i<numInfoWindows;i++)
    {
      GetInfoBoxPosition(i, rc, &xoff, &yoff, &sizex, &sizey);

      InfoBoxes[i] = new InfoBox(hWndMainWindow, xoff, yoff, sizex, sizey);

      int Border=0;
      if (gnav){
        if (i>0)
          Border |= BORDERTOP;
        if (i<6)
          Border |= BORDERRIGHT;
        InfoBoxes[i]->SetBorderKind(Border);
      } else
      if (!landscape) {
        Border = 0;
        if (i<4) {
          Border |= BORDERBOTTOM;
        } else {
          Border |= BORDERTOP;
        }
        Border |= BORDERRIGHT;
        InfoBoxes[i]->SetBorderKind(Border);
      }
    }

}

void InfoBoxLayout::DestroyInfoBoxes(void){
  int i;
  for(i=0; i<numInfoWindows+1; i++){
    delete (InfoBoxes[i]);
  }

}

