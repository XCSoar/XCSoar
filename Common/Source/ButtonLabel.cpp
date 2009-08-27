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
#include "XCSoar.h"
#include "ButtonLabel.h"
#include "LogFile.hpp"
#include "Dialogs.h"
#include "Screen/Animation.hpp"
#include "Registry.hpp"
#include "InfoBox.h"
#include "WindowControls.h"
#include "ExpandMacros.hpp"
#include "Interface.hpp"
#include "Compatibility/string.h"

#ifdef PNA
#include "Asset.hpp"
#endif

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

  _stprintf(reggeompx, TEXT("ScreenButtonPosX%d"), i);
  _stprintf(reggeompy, TEXT("ScreenButtonPosY%d"), i);
  _stprintf(reggeomsx, TEXT("ScreenButtonSizeX%d"), i);
  _stprintf(reggeomsy, TEXT("ScreenButtonSizeY%d"), i);

  GetFromRegistry(reggeompx,&Temp); *x = Temp;
  GetFromRegistry(reggeompy,&Temp); *y = Temp;
  GetFromRegistry(reggeomsx,&Temp); *sizex = Temp;
  GetFromRegistry(reggeomsy,&Temp); *sizey = Temp;

  bool geometrychanged = true; // JMW testing

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

  if (InfoBoxLayout::gnav) {
    ButtonLabelGeometry = 1;
  } else {
    ButtonLabelGeometry = 0;
  }

  for (i=0; i<NUMBUTTONLABELS; i++) {
// VENTA3 added THICKFRAME
#ifdef PNA // VENTA3 FIX  better borders
 if (GlobalModelType == MODELTYPE_PNA_HP31X )
    hWndButtonWindow[i] =
      CreateWindowEx( WS_EX_CLIENTEDGE,
		   TEXT("STATIC"), TEXT("\0"),
		   WS_CHILD|WS_TABSTOP|WS_THICKFRAME
		   |SS_CENTER|SS_NOTIFY
		   |WS_CLIPCHILDREN | WS_CLIPSIBLINGS | WS_BORDER,
		   rc.left, rc.top,
		   buttonWidth, buttonHeight,
		   hWndMainWindow, NULL, hInst, NULL);
 else
    hWndButtonWindow[i] =
      CreateWindow(
		   TEXT("STATIC"), TEXT("\0"),
		   /*WS_VISIBLE|*/ WS_CHILD|WS_TABSTOP
		   |SS_CENTER|SS_NOTIFY
		   |WS_CLIPCHILDREN | WS_CLIPSIBLINGS | WS_BORDER,
		   rc.left, rc.top,
		   // TODO code: need to have these passed in too as
		   // some buttons may actually be a different shape.
		   buttonWidth, buttonHeight,
		   hWndMainWindow, NULL, hInst, NULL);

#else
    hWndButtonWindow[i] =
      CreateWindow(
		   TEXT("STATIC"), TEXT("\0"),
		   /*WS_VISIBLE|*/ WS_CHILD|WS_TABSTOP
		   |SS_CENTER|SS_NOTIFY
		   |WS_CLIPCHILDREN | WS_CLIPSIBLINGS | WS_BORDER,
		   rc.left, rc.top,
		   // TODO code: need to have these passed in too as
		   // some buttons may actually be a different shape.
		   buttonWidth, buttonHeight,
		   hWndMainWindow, NULL, hInst, NULL);
#endif

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


void ButtonLabel::SetLabelText(int index, const TCHAR *text) {
  // error! TODO enhancement: Add debugging
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
