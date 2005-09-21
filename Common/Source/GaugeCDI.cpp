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

#include "GaugeCDI.h"

HWND hWndCDIWindow = NULL; //CDI Window
extern HWND hWndMainWindow; // Main Windows
extern HWND hWndMenuButton;
extern HINSTANCE hInst;      // The current instance

#include "InfoBoxLayout.h"

extern HFONT CDIWindowFont; // New

void GaugeCDI::Create() {
  // start of new code for displaying CDI window
  RECT rc;
  GetClientRect(hWndMainWindow, &rc);

  hWndCDIWindow = CreateWindow(TEXT("STATIC"),TEXT(" "),
			       WS_VISIBLE|WS_CHILD | WS_CLIPCHILDREN 
			       | WS_CLIPSIBLINGS,
                               0,0,0,0,
			       hWndMainWindow,NULL,hInst,NULL);

  SendMessage(hWndCDIWindow,WM_SETFONT,
              (WPARAM)CDIWindowFont,MAKELPARAM(TRUE,0));
  SetWindowPos(hWndCDIWindow,hWndMenuButton,
               (int)(InfoBoxLayout::ControlWidth*0.6),
	       (int)(InfoBoxLayout::ControlHeight+1),
               (int)(InfoBoxLayout::ControlWidth*2.8),
	       (int)(InfoBoxLayout::TitleHeight*1.4),
	       SWP_SHOWWINDOW);

  // end of new code for drawing CDI window (see below for destruction of objects)

  ShowWindow(hWndCDIWindow, SW_HIDE);

}


void GaugeCDI::Destroy() {
  DestroyWindow(hWndCDIWindow);
}
