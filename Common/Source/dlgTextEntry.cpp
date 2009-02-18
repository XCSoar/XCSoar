
/*
Copyright_License {

  XCSoar Glide Computer - http://xcsoar.sourceforge.net/
  Copyright (C) 2000 - 2008  

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

#include "StdAfx.h"
#include "XCSoar.h"
#include "Utils.h"
#include "dlgTools.h"
#include "externs.h"

static WndForm *wf=NULL;
static WndOwnerDrawFrame *wGrid=NULL;

#define MAX_TEXTENTRY 40
static unsigned int cursor = 0;
static int lettercursor=0;
static int max_width = MAX_TEXTENTRY;

static TCHAR edittext[MAX_TEXTENTRY];

static TCHAR EntryLetters[] = TEXT(" ABCDEFGHIJKLMNOPQRSTUVWXYZ1234567890.-");

#define MAXENTRYLETTERS (sizeof(EntryLetters)/sizeof(EntryLetters[0])-1)

static void OnCloseClicked(WindowControl * Sender){
	(void)Sender;
  wf->SetModalResult(mrOK);
}


static void OnTextPaint(WindowControl *Sender, HDC hDC) {
  RECT  rcgfx;
  HFONT hfOld;

  CopyRect(&rcgfx, Sender->GetBoundRect());
  // background is painted in the base-class
  hfOld = (HFONT)SelectObject(hDC, Sender->GetFont());
  SetBkMode(hDC, TRANSPARENT);
  SetTextColor(hDC, Sender->GetForeColor());

  ////
  // Do the actual painting of the text

  SIZE tsize;
  GetTextExtentPoint(hDC, edittext, _tcslen(edittext), &tsize);
  SIZE tsizec;
  GetTextExtentPoint(hDC, edittext, cursor, &tsizec);
  SIZE tsizea;
  GetTextExtentPoint(hDC, edittext, cursor+1, &tsizea);
  
  POINT p[5];
  p[0].x = 10;
  p[0].y = 20;

  p[2].x = p[0].x + tsizec.cx;
  p[2].y = p[0].y + tsize.cy+5;

  p[3].x = p[0].x + tsizea.cx;
  p[3].y = p[0].y + tsize.cy+5;

  p[1].x = p[2].x;
  p[1].y = p[2].y-2;

  p[4].x = p[3].x;
  p[4].y = p[3].y-2;

  SelectObject(hDC, GetStockObject(WHITE_PEN));
  Polyline(hDC, p+1, 4);

  /*
  int x = (int)((xv-x_min)*xscale)+rc.left-tsize.cx/2;
  int y = (int)((y_max-yv)*yscale)+rc.top-tsize.cy/2;
  */
  SetBkMode(hDC, OPAQUE);
  ExtTextOut(hDC, p[0].x, p[0].y, ETO_OPAQUE, NULL, 
             edittext, _tcslen(edittext), NULL);
  SetBkMode(hDC, TRANSPARENT);

  ////

  SelectObject(hDC, hfOld);
}



static void UpdateCursor(void) {
  if (lettercursor>=MAXENTRYLETTERS)
    lettercursor = 0;
  if (lettercursor<0)
    lettercursor = MAXENTRYLETTERS-1;
  edittext[cursor] = EntryLetters[lettercursor];

  if (wGrid != NULL)
    wGrid->Redraw();

}


static void MoveCursor(void) {
  if (cursor>=_tcslen(edittext)) {
    edittext[cursor+1] = 0;
  }
  for (lettercursor=0; lettercursor< MAXENTRYLETTERS; lettercursor++) {
    if (edittext[cursor]== EntryLetters[lettercursor])
      break;
  }
  if (lettercursor== MAXENTRYLETTERS) {
    lettercursor = 0;
    edittext[cursor] = EntryLetters[lettercursor];
  }
  if (edittext[cursor]== 0) {
    lettercursor= 0;
  }
  UpdateCursor();
}


static int FormKeyDown(WindowControl * Sender, WPARAM wParam, LPARAM lParam) {
	(void)lParam; (void)Sender;
  switch(wParam & 0xffff){
    case VK_LEFT:
      if (cursor<1)
        return(0); // min width
      cursor--;
      MoveCursor();
      return(0);
    case VK_RIGHT:
      if ((int)cursor>=(max_width-2))
        return(0); // max width
      cursor++;
      MoveCursor();
      return(0);
    case VK_UP:
      lettercursor--;
      UpdateCursor();
      return(0);
    case VK_DOWN:
      lettercursor++;
      UpdateCursor();
#ifdef VENTA_DEBUG_EVENT
	  DoStatusMessage(TEXT("DBG dlgTxtEntry VK DOWN 2")); // VENTA
#endif
      return(0);
    case VK_RETURN:
      wf->SetModalResult(mrOK);
      return(0);
  }
  return(1);
}



static CallBackTableEntry_t CallBackTable[]={
  DeclareCallBackEntry(OnTextPaint),
  DeclareCallBackEntry(NULL)
};


static void OnLeftClicked(WindowControl * Sender){
  (void)Sender;
  FormKeyDown(Sender, VK_LEFT, 0);
}

static void OnRightClicked(WindowControl * Sender){
  (void)Sender;
  FormKeyDown(Sender, VK_RIGHT, 0);
}

static void OnUpClicked(WindowControl * Sender){
  (void)Sender;
  FormKeyDown(Sender, VK_UP, 0);
}

static void OnDownClicked(WindowControl * Sender){
  (void)Sender;
  FormKeyDown(Sender, VK_DOWN, 0);
#ifdef VENTA_DEBUG_EVENT
   DoStatusMessage(TEXT("DBG dlgTxtEntry VK DOWN 1")); // VENTA
#endif
}



void dlgTextEntryHighscoreType(TCHAR *text, int width)
{
  wf = NULL;
  wGrid = NULL;

  if (width==0) {
    width = MAX_TEXTENTRY;
  }
  max_width = min(MAX_TEXTENTRY, width);

  char filename[MAX_PATH];
#ifndef GNAV
  LocalPathS(filename, TEXT("dlgTextEntry_T.xml"));
  wf = dlgLoadFromXML(CallBackTable, 
                      filename, 
		      hWndMainWindow,
		      TEXT("IDR_XML_TEXTENTRY_T"));
#else
  LocalPathS(filename, TEXT("dlgTextEntry.xml"));
  wf = dlgLoadFromXML(CallBackTable, 
                      filename, 
		      hWndMainWindow,
		      TEXT("IDR_XML_TEXTENTRY"));
#endif
  if (!wf) return;

  wGrid = (WndOwnerDrawFrame*)wf->FindByName(TEXT("frmGrid"));

  WndButton* wb;
  wb = (WndButton *)(wf->FindByName(TEXT("cmdClose")));
  if (wb) {
    wb->SetOnClickNotify(OnCloseClicked);
  }

  wb = (WndButton *)(wf->FindByName(TEXT("cmdLeft")));
  if (wb) {
    wb->SetOnClickNotify(OnLeftClicked);
  }

  wb = (WndButton *)(wf->FindByName(TEXT("cmdRight")));
  if (wb) {
    wb->SetOnClickNotify(OnRightClicked);
  }

  wb = (WndButton *)(wf->FindByName(TEXT("cmdUp")));
  if (wb) {
    wb->SetOnClickNotify(OnUpClicked);
  }

  wb = (WndButton *)(wf->FindByName(TEXT("cmdDown")));
  if (wb) {
    wb->SetOnClickNotify(OnDownClicked);
  }


  cursor = 0;
  edittext[0]= 0;
  edittext[1]= 0;
  if (_tcslen(text)>0) {
    _tcsupr(text);
    _tcsncpy(edittext, text, max_width-1);
    edittext[max_width-1]= 0;
  }
  MoveCursor();

  wf->SetKeyDownNotify(FormKeyDown);

  wf->ShowModal();

  _tcsncpy(text, edittext, max_width);
  text[max_width-1]=0;

  // strip trailing spaces
  int len = _tcslen(text)-1;
  while ((len>0) && (text[len] == _T(' '))) {
    text[len] = 0;
    len--;
  }

  delete wf;
}


void dlgTextEntryShowModal(TCHAR *text, int width) 
{
  switch (Appearance.TextInputStyle)
    {
    case tiKeyboard:
      dlgTextEntryKeyboardShowModal(text, width);	
      break;
    case tiHighScore:
    default:
      dlgTextEntryHighscoreType(text, width);
      break;
    }
}
