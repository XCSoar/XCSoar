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


#include "StdAfx.h"
#include "XCSoar.h"

#include "externs.h"
#include "dlgTools.h"
#include "InfoBoxLayout.h"
#include "MapWindow.h"
#include "Utils.h"

#include <assert.h>

static WndForm *wf=NULL;
static WndListFrame *wAirspaceColoursList=NULL;
static WndOwnerDrawFrame *wAirspaceColoursListEntry = NULL;

static int ItemIndex = -1;


static void UpdateList(void){
  wAirspaceColoursList->ResetList();
  wAirspaceColoursList->Redraw();
}

static int DrawListIndex=0;

static void OnAirspaceColoursPaintListItem(WindowControl * Sender, HDC hDC){
  (void)Sender;
  if ((DrawListIndex < NUMAIRSPACECOLORS) &&(DrawListIndex>=0)) {
    int i = DrawListIndex;
    SelectObject(hDC, GetStockObject(WHITE_BRUSH));
    SelectObject(hDC, GetStockObject(BLACK_PEN));
    SetBkColor(hDC,
	       RGB(0xFF, 0xFF, 0xFF));
    SelectObject(hDC,
		 MapWindow::GetAirspaceBrush(1)); // this is the solid brush
    SetTextColor(hDC,
		 MapWindow::GetAirspaceColour(i));
    Rectangle(hDC,
              100*InfoBoxLayout::scale,
              2*InfoBoxLayout::scale,
              180*InfoBoxLayout::scale,
              22*InfoBoxLayout::scale);
  }
}


static void OnAirspaceColoursListEnter(WindowControl * Sender,
				WndListFrame::ListInfo_t *ListInfo) {
  (void)Sender;
  ItemIndex = ListInfo->ItemIndex + ListInfo->ScrollIndex;
  if (ItemIndex>=NUMAIRSPACECOLORS) {
    ItemIndex = NUMAIRSPACECOLORS-1;
  }
  if (ItemIndex>=0) {
    wf->SetModalResult(mrOK);
  }
}


static void OnAirspaceColoursListInfo(WindowControl * Sender,
			       WndListFrame::ListInfo_t *ListInfo){
  (void)Sender;
  if (ListInfo->DrawIndex == -1){
    ListInfo->ItemCount = NUMAIRSPACECOLORS;
  } else {
    DrawListIndex = ListInfo->DrawIndex+ListInfo->ScrollIndex;
    ItemIndex = ListInfo->ItemIndex+ListInfo->ScrollIndex;
  }
}

static void OnCloseClicked(WindowControl * Sender){
  (void)Sender;
  ItemIndex = -1;
  wf->SetModalResult(mrOK);
}


static CallBackTableEntry_t CallBackTable[]={
  DeclareCallBackEntry(OnAirspaceColoursPaintListItem),
  DeclareCallBackEntry(OnAirspaceColoursListInfo),
  DeclareCallBackEntry(OnCloseClicked),
  DeclareCallBackEntry(NULL)
};


int dlgAirspaceColoursShowModal(void){

  ItemIndex = -1;

  if (!InfoBoxLayout::landscape) {
    wf = dlgLoadFromXML(CallBackTable,
                        TEXT("dlgAirspaceColours_L.xml"),
                        hWndMainWindow,
                        TEXT("IDR_XML_AIRSPACECOLOURS_L"));
  } else {
    wf = dlgLoadFromXML(CallBackTable,
                        TEXT("dlgAirspaceColours.xml"),
                        hWndMainWindow,
                        TEXT("IDR_XML_AIRSPACECOLOURS"));
  }

  if (!wf) return -1;

  assert(wf!=NULL);

  wAirspaceColoursList = (WndListFrame*)wf->FindByName(TEXT("frmAirspaceColoursList"));
  assert(wAirspaceColoursList!=NULL);
  wAirspaceColoursList->SetBorderKind(BORDERLEFT);
  wAirspaceColoursList->SetEnterCallback(OnAirspaceColoursListEnter);

  wAirspaceColoursListEntry = (WndOwnerDrawFrame*)wf->
    FindByName(TEXT("frmAirspaceColoursListEntry"));
  assert(wAirspaceColoursListEntry!=NULL);
  wAirspaceColoursListEntry->SetCanFocus(true);

  UpdateList();

  wf->ShowModal();

  // now retrieve back the properties...

  delete wf;

  wf = NULL;

  return ItemIndex;
}


