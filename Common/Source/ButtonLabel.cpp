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
#include "Language.hpp"
#include "Screen/Animation.hpp"
#include "Registry.hpp"
#include "InfoBox.h"
#include "WindowControls.h"
#include "Interface.hpp"
#include "Compatibility/string.h"
#include "options.h" /* for IBLSCALE() */

#ifdef PNA
#include "Asset.hpp"
#endif

TextWindow ButtonLabel::hWndButtonWindow[NUMBUTTONLABELS];
bool ButtonLabel::ButtonVisible[NUMBUTTONLABELS];
bool ButtonLabel::ButtonDisabled[NUMBUTTONLABELS];

unsigned ButtonLabel::ButtonLabelGeometry = 0;


void
ButtonLabel::GetButtonPosition(unsigned i, RECT rc,
                               int *x, int *y,
                               int *sizex, int *sizey)
{
  TCHAR reggeompx[50];
  TCHAR reggeompy[50];
  TCHAR reggeomsx[50];
  TCHAR reggeomsy[50];

  _stprintf(reggeompx, TEXT("ScreenButtonPosX%d"), i);
  _stprintf(reggeompy, TEXT("ScreenButtonPosY%d"), i);
  _stprintf(reggeomsx, TEXT("ScreenButtonSizeX%d"), i);
  _stprintf(reggeomsy, TEXT("ScreenButtonSizeY%d"), i);

  GetFromRegistry(reggeompx,*x);
  GetFromRegistry(reggeompy,*y); 
  GetFromRegistry(reggeomsx,*sizex);
  GetFromRegistry(reggeomsy,*sizey);

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


void
ButtonLabel::CreateButtonLabels(ContainerWindow &parent, const RECT rc)
{
  int x, y, xsize, ysize;

  if (InfoBoxLayout::gnav) {
    ButtonLabelGeometry = 1;
  } else {
    ButtonLabelGeometry = 0;
  }

  for (unsigned i = 0; i < NUMBUTTONLABELS; i++) {
    GetButtonPosition(i, rc, &x, &y, &xsize, &ysize);
    hWndButtonWindow[i].set(parent, x, y, xsize, ysize,
                            true, true, false, true, true);
    hWndButtonWindow[i].insert_after(HWND_TOP, true);

    ButtonVisible[i]= true;
    ButtonDisabled[i]= false;

    SetLabelText(i,NULL);
    hWndButtonWindow[i].set_userdata(4);
  }
  //
}

void ButtonLabel::SetFont(const Font &Font) {
  for (unsigned i = 0; i < NUMBUTTONLABELS; i++) {
    hWndButtonWindow[i].set_font(Font);
  }
}


void ButtonLabel::Destroy() {
  for (unsigned i = 0; i < NUMBUTTONLABELS; i++) {
    hWndButtonWindow[i].reset();
    ButtonVisible[i]= false;
    ButtonDisabled[i] = true;
  }
}


void
ButtonLabel::SetLabelText(unsigned index, const TCHAR *text)
{
  // error! TODO enhancement: Add debugging
  if (index>= NUMBUTTONLABELS)
    return;

  // don't try to draw if window isn't initialised
  if (hWndButtonWindow[index] == NULL)
    return;

  if ((text==NULL) || (*text==_T('\0'))||(*text==_T(' '))) {
    hWndButtonWindow[index].hide();
    ButtonVisible[index]= false;
  } else {

    TCHAR s[100];

    bool greyed = ExpandMacros(text, s, sizeof(s)/sizeof(s[0]));

    if (greyed) {
      hWndButtonWindow[index].set_userdata(5);
      ButtonDisabled[index]= true;
    } else {
      hWndButtonWindow[index].set_userdata(4);
      ButtonDisabled[index]= false;
    }

    if ((s[0]==_T('\0'))||(s[0]==_T(' '))) {
      hWndButtonWindow[index].hide();
      ButtonVisible[index]= false;
    } else {
      hWndButtonWindow[index].set_text(gettext(s));
      hWndButtonWindow[index].insert_after(HWND_TOP, true);

      ButtonVisible[index]= true;
    }
  }

}

#include "InputEvents.h"

bool ButtonLabel::CheckButtonPress(HWND pressedwindow) {
  for (unsigned i = 0; i < NUMBUTTONLABELS; i++) {
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

void
ButtonLabel::AnimateButton(unsigned i)
{
  RECT mRc, aniRect;
  mRc = hWndButtonWindow[i].get_position();

  if (ButtonVisible[i]) {
    aniRect.top = (mRc.top*5+mRc.bottom)/6;
    aniRect.left = (mRc.left*5+mRc.right)/6;
    aniRect.right = (mRc.left+mRc.right*5)/6;
    aniRect.bottom = (mRc.top+mRc.bottom*5)/6;
    SetSourceRectangle(aniRect);
    DrawWireRects(EnableAnimation, &mRc, 5);
  }

  SetSourceRectangle(mRc);

}
