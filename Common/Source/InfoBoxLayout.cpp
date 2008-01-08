/*
Copyright_License {

  XCSoar Glide Computer - http://xcsoar.sourceforge.net/
  Copyright (C) 2000 - 2005

  	M Roberts (original release)
	Robin Birch <robinb@ruffnready.co.uk>
	Samuel Gisiger <samuel.gisiger@triadis.ch>
	Jeff Goodenough <jeff@enborne.f2s.com>
	Alastair Harrison <aharrison@magic.force9.co.uk>
	Scott Penrose <scottp@dd.com.au>
	John Wharington <jwharington@bigfoot.com>

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
#include "stdafx.h"
#include "Sizes.h"
#include "MapWindow.h"
#include "InfoBoxLayout.h"
#include "Dialogs.h"
#include "Utils.h"
#include "externs.h"

#include "infobox.h"

extern InfoBox *InfoBoxes[MAXINFOWINDOWS];
extern HWND hWndMainWindow; // Main Windows
extern HINSTANCE hInst; // The current instance

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

bool gnav = false;

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

  wsprintf(reggeompx, TEXT("InfoBoxPositionPosX%d"), i);
  wsprintf(reggeompy, TEXT("InfoBoxPositionPosY%d"), i);
  wsprintf(reggeomsx, TEXT("InfoBoxPositionSizeX%d"), i);
  wsprintf(reggeomsy, TEXT("InfoBoxPositionSizeY%d"), i);

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


void InfoBoxLayout::ScreenGeometry(RECT rc) {

  TCHAR szRegistryInfoBoxGeometry[]=  TEXT("InfoBoxGeometry");
  DWORD Temp=0;
  GetFromRegistry(szRegistryInfoBoxGeometry,&Temp);
  InfoBoxGeometry = Temp;

  // JMW testing only
  geometrychanged = true;

  int maxsize=0;
  int minsize=0;
  maxsize = max(rc.right-rc.left,rc.bottom-rc.top);
  minsize = min(rc.right-rc.left,rc.bottom-rc.top);
  if (maxsize==minsize) {
    scale = max(1,minsize/240);
  } else {
    scale = max(1,maxsize/320);
  }

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

  case 1:
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

  case 2:
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

  case 3:
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

  case 5:
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

    ControlWidth = (int)((rc.right - rc.left)*0.18);
    ControlHeight = (int)((rc.bottom - rc.top)/6);
    TitleHeight = (int)(ControlHeight/TITLEHEIGHTRATIO);

    // calculate small map screen size

    MapWindow::MapRect.top = rc.top;
    MapWindow::MapRect.left = rc.left;
    MapWindow::MapRect.bottom = rc.bottom;
    MapWindow::MapRect.right = rc.right-ControlWidth*2;

    break;

  case 7:
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

///////////////////////

HWND ButtonLabel::hWndButtonWindow[NUMBUTTONLABELS];
bool ButtonLabel::ButtonVisible[NUMBUTTONLABELS];
bool ButtonLabel::ButtonDisabled[NUMBUTTONLABELS];

int ButtonLabel::ButtonLabelGeometry = 0;


void ButtonLabel::GetButtonPosition(int i, RECT rc,
				    int *x, int *y,
				    int *sizex, int *sizey) {

  TCHAR reggeompx[50];
  TCHAR reggeompy[50];
  TCHAR reggeomsx[50];
  TCHAR reggeomsy[50];
  DWORD Temp=0;

  wsprintf(reggeompx, TEXT("ScreenButtonPosX%d"), i);
  wsprintf(reggeompy, TEXT("ScreenButtonPosY%d"), i);
  wsprintf(reggeomsx, TEXT("ScreenButtonSizeX%d"), i);
  wsprintf(reggeomsy, TEXT("ScreenButtonSizeY%d"), i);

  GetFromRegistry(reggeompx,&Temp); *x = Temp;
  GetFromRegistry(reggeompy,&Temp); *y = Temp;
  GetFromRegistry(reggeomsx,&Temp); *sizex = Temp;
  GetFromRegistry(reggeomsy,&Temp); *sizey = Temp;

  if ((*sizex==0)||(*sizey==0)||geometrychanged) {
    // not defined in registry so go with defaults
    // these will be saved back to registry
    int hwidth = (rc.right-rc.left)/4;
    int hheight = (rc.bottom-rc.top)/4;

    switch (ButtonLabelGeometry) {
    case 0: // portrait
      if (i==0) {
	*sizex = IBLSCALE(52);
	*sizey = IBLSCALE(37);
	*x = rc.left-(*sizex); // JMW make it offscreen for now
	*y = (rc.bottom-(*sizey));
      } else {
        if (i<5) {
          *sizex = IBLSCALE(52);
          *sizey = IBLSCALE(40);
          *x = rc.left+3+hwidth*(i-1);
          *y = (rc.bottom-(*sizey));
        } else {
          *sizex = IBLSCALE(80);
          *sizey = IBLSCALE(40);
          *x = rc.right-(*sizex);
          int k = rc.bottom-rc.top-IBLSCALE(46);
#ifdef GNAV
          k = rc.bottom-rc.top;
          // JMW need upside down button order for rotated Altair
          *y = rc.bottom-(i-5)*k/5-(*sizey)-IBLSCALE(20);
#else
          *y = (rc.top+(i-5)*k/6+(*sizey/2+IBLSCALE(3)));
#endif
        }
      }
      break;

    case 1: // landscape
      hwidth = (rc.right-rc.left)/5;
      hheight = (rc.bottom-rc.top)/(4+1);

      if (i==0) {
	*sizex = IBLSCALE(52);
	*sizey = IBLSCALE(20);
	*x = rc.left-(*sizex); // JMW make it offscreen for now
	*y = (rc.top);
      } else {
	if (i<5) {
	  *sizex = IBLSCALE(52);
#ifdef GNAV
	  *sizey = IBLSCALE(20);
#else
	  *sizey = IBLSCALE(35);
#endif
	  *x = rc.left+3;
	  *y = (rc.top+hheight*i-(*sizey)/2);
	} else {
	  *sizex = IBLSCALE(60);
#ifdef GNAV
	  *sizey = IBLSCALE(40);
#else
	  *sizey = IBLSCALE(35);
#endif
	  *x = rc.left+hwidth*(i-5);
	  *y = (rc.bottom-(*sizey));
	}
      }
      break;

    }

    SetToRegistry(reggeompx,*x);
    SetToRegistry(reggeompy,*y);
    SetToRegistry(reggeomsx,*sizex);
    SetToRegistry(reggeomsy,*sizey);

  };

}


void ButtonLabel::CreateButtonLabels(RECT rc) {
  int i;
  int x, y, xsize, ysize;

  int buttonWidth = IBLSCALE(50);
  int buttonHeight = IBLSCALE(15);

  if (gnav) {
    ButtonLabelGeometry = 1;
  } else {
    ButtonLabelGeometry = 0;
  }

  for (i=0; i<NUMBUTTONLABELS; i++) {
    hWndButtonWindow[i] =
      CreateWindow(
		   TEXT("STATIC"), TEXT("\0"),
		   /*WS_VISIBLE|*/ WS_CHILD|WS_TABSTOP
		   |SS_CENTER|SS_NOTIFY
		   |WS_CLIPCHILDREN | WS_CLIPSIBLINGS | WS_BORDER,
		   rc.left, rc.top,
		   // TODO need to have these passed in too as some buttons
		   // may actually be a different shape.
		   buttonWidth, buttonHeight,
		   hWndMainWindow, NULL, hInst, NULL);
    GetButtonPosition(i, rc, &x, &y, &xsize, &ysize);

    SetWindowPos(hWndButtonWindow[i],HWND_TOP,
		 x, y,
		 xsize, ysize, SWP_SHOWWINDOW);
    ButtonVisible[i]= true;
    ButtonDisabled[i]= false;

    SetLabelText(i,NULL);
    SetWindowLong(hWndButtonWindow[i], GWL_USERDATA, 4);
  }

  //

  if (gnav) {
    EnableVarioGauge = true;
  } else {
    EnableVarioGauge = false;
  }

}

void ButtonLabel::SetFont(HFONT Font) {
  int i;
  for (i=0; i<NUMBUTTONLABELS; i++) {
    SendMessage(hWndButtonWindow[i], WM_SETFONT,
              (WPARAM)Font, MAKELPARAM(TRUE,0));
  }
}


void ButtonLabel::Destroy() {
  int i;
  for (i=0; i<NUMBUTTONLABELS; i++) {
    DestroyWindow(hWndButtonWindow[i]);

    // prevent setting of button details if it's been destroyed
    hWndButtonWindow[i] = NULL;
    ButtonVisible[i]= false;
    ButtonDisabled[i] = true;
  }
}


void ButtonLabel::SetLabelText(int index, TCHAR *text) {
  // error! TODO Add debugging
  if (index>= NUMBUTTONLABELS)
    return;

  // don't try to draw if window isn't initialised
  if (hWndButtonWindow[index] == NULL)
    return;

  if ((text==NULL) || (*text==_T('\0'))||(*text==_T(' '))) {
    ShowWindow(hWndButtonWindow[index], SW_HIDE);
    ButtonVisible[index]= false;
  } else {

    TCHAR s[100];

    bool greyed = ExpandMacros(text, s, sizeof(s)/sizeof(s[0]));

    if (greyed) {
      SetWindowLong(hWndButtonWindow[index], GWL_USERDATA, 5);
      ButtonDisabled[index]= true;
    } else {
      SetWindowLong(hWndButtonWindow[index], GWL_USERDATA, 4);
      ButtonDisabled[index]= false;
    }

    if ((s[0]==_T('\0'))||(s[0]==_T(' '))) {
      ShowWindow(hWndButtonWindow[index], SW_HIDE);
      ButtonVisible[index]= false;
    } else {

      SetWindowText(hWndButtonWindow[index], gettext(s));

      SetWindowPos(hWndButtonWindow[index], HWND_TOP, 0,0,0,0,
                   SWP_NOMOVE | SWP_NOSIZE);

      ShowWindow(hWndButtonWindow[index], SW_SHOW);
      ButtonVisible[index]= true;
    }
  }

}

#include "InputEvents.h"

bool ButtonLabel::CheckButtonPress(HWND pressedwindow) {
  int i;
  for (i=0; i<NUMBUTTONLABELS; i++) {
    if (hWndButtonWindow[i]== pressedwindow) {
      if (!ButtonDisabled[i]) {
        InputEvents::processButton(i);
        return TRUE;
      } else {
        return FALSE;
      }
      return FALSE;
    }
  }
  return FALSE;
}


void ButtonLabel::AnimateButton(int i) {
  RECT mRc, aniRect;
  GetWindowRect(hWndButtonWindow[i], &mRc);

  if (ButtonVisible[i]) {
    aniRect.top = (mRc.top*5+mRc.bottom)/6;
    aniRect.left = (mRc.left*5+mRc.right)/6;
    aniRect.right = (mRc.left+mRc.right*5)/6;
    aniRect.bottom = (mRc.top+mRc.bottom*5)/6;
    SetSourceRectangle(aniRect);
    DrawWireRects(&mRc, 5);
  }

  SetSourceRectangle(mRc);

}
