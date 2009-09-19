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

#include "Dialogs/dlgTools.h"
#include "Language.hpp"
#include "XCSoar.h"
#include "WindowControls.h"
#include "InfoBoxLayout.h"
#include "Dialogs.h"
#include "MainWindow.hpp"
#include "Screen/Fonts.hpp"

#include <assert.h>
#include <limits.h>

int DLGSCALE(int x) {
  int iRetVal = x;

#ifndef ALTAIRSYNC
    iRetVal = (int) ((x)*InfoBoxLayout::dscale);
#endif
  return iRetVal;
}

// Message Box Replacement
/*
    MessageBox(hWndMainWindow,
      gettext(TEXT("Too many waypoints in task!")),
      gettext(TEXT("Insert Waypoint")),
      MB_OK|MB_ICONEXCLAMATION);
*/

static void OnButtonClick(WindowControl * Sender){
  ((WndForm *)Sender->GetOwner()->GetOwner())->SetModalResult(Sender->GetTag());
}

int WINAPI MessageBoxX(LPCTSTR lpText, LPCTSTR lpCaption, UINT uType){

  WndForm *wf=NULL;
  WndFrame *wText=NULL;
  int X, Y, Width, Height;
  WndButton *wButtons[10];
  int ButtonCount = 0;
  int i,x,y,d,w,h,res,dY;
  RECT rc;

  // todo

  assert(lpText != NULL);
  assert(lpCaption != NULL);

  // JMW this makes the first key if pressed quickly, ignored
  XCSoarInterface::Debounce();

  rc = XCSoarInterface::main_window.get_position();

#ifdef ALTAIRSYNC
  Width = DLGSCALE(220);
  Height = DLGSCALE(160);
#else
  Width = DLGSCALE(200);
  Height = DLGSCALE(160);
#endif

  X = ((rc.right-rc.left) - Width)/2;
  Y = ((rc.bottom-rc.top) - Height)/2;

  y = DLGSCALE(100);
  w = DLGSCALE(60);
  h = DLGSCALE(32);

  wf = new WndForm(&XCSoarInterface::main_window, TEXT("frmXcSoarMessageDlg"),
                   lpCaption, X, Y, Width, Height);
  wf->SetFont(MapWindowBoldFont);
  wf->SetTitleFont(MapWindowBoldFont);
  wf->SetBackColor(Color(0xDA, 0xDB, 0xAB));

  wText = new WndFrame(wf,
                       TEXT("frmMessageDlgText"),
                       0,
                       DLGSCALE(5),
                       Width,
                       Height);
  wText->SetCaption(lpText);
  wText->SetFont(MapWindowBoldFont);
  wText->SetCaptionStyle(
        DT_EXPANDTABS
      | DT_CENTER
      | DT_NOCLIP
      | DT_WORDBREAK
        //      | DT_VCENTER
  );

  /* TODO code: this doesnt work to set font height
  dY = wText->GetLastDrawTextHeight() - Height;
  */
  dY = DLGSCALE(-40);
  wText->SetHeight(wText->GetTextHeight() + 5);
  wf->SetHeight(wf->GetHeight() + dY);

  y += dY;

  uType = uType & 0x000f;

  if (uType == MB_OK
      || uType == MB_OKCANCEL

  ){
    wButtons[ButtonCount] = new WndButton(wf, TEXT(""), gettext(TEXT("OK")),
                                          0, y, w, h, OnButtonClick);
    wButtons[ButtonCount]->SetTag(IDOK);
    ButtonCount++;
  }

  if (uType == MB_YESNO
      || uType == MB_YESNOCANCEL
  ){
    wButtons[ButtonCount] = new WndButton(wf, TEXT(""), gettext(TEXT("Yes")),
                                          0, y, w, h, OnButtonClick);
    wButtons[ButtonCount]->SetTag(IDYES);
    ButtonCount++;
    wButtons[ButtonCount] = new WndButton(wf, TEXT(""), gettext(TEXT("No")),
                                          0, y, w, h, OnButtonClick);
    wButtons[ButtonCount]->SetTag(IDNO);
    ButtonCount++;
  }

  if (uType == MB_ABORTRETRYIGNORE
      || uType == MB_RETRYCANCEL
  ){
    wButtons[ButtonCount] = new WndButton(wf, TEXT(""),
                                          gettext(TEXT("Retry")),
                                          0, y, w, h, OnButtonClick);
    wButtons[ButtonCount]->SetTag(IDRETRY);
    ButtonCount++;
  }

  if (uType == MB_OKCANCEL
      || uType == MB_RETRYCANCEL
      || uType == MB_YESNOCANCEL
  ){
    wButtons[ButtonCount] = new WndButton(wf, TEXT(""),
                                          gettext(TEXT("Cancel")),
                                          0, y, w, h, OnButtonClick);
    wButtons[ButtonCount]->SetTag(IDCANCEL);
    ButtonCount++;
  }

  if (uType == MB_ABORTRETRYIGNORE
  ){
    wButtons[ButtonCount] = new WndButton(wf, TEXT(""),
                                          gettext(TEXT("Abort")),
                                          0, y, w, h, OnButtonClick);
    wButtons[ButtonCount]->SetTag(IDABORT);
    ButtonCount++;
    wButtons[ButtonCount] = new WndButton(wf, TEXT(""),
                                          gettext(TEXT("Ignore")),
                                          0, y, w, h, OnButtonClick);
    wButtons[ButtonCount]->SetTag(IDIGNORE);
    ButtonCount++;
  }

  d = Width / (ButtonCount);
  x = d/2-w/2;

  for (i=0; i<ButtonCount; i++){
    wButtons[i]->SetLeft(x);
    x += d;
  }

  res = wf->ShowModal();

  delete wf;

#ifdef ALTAIRSYNC
  // force a refresh of the window behind
  InvalidateRect(hWnd,NULL,true);
  UpdateWindow(hWnd);
#endif
  return(res);

}
