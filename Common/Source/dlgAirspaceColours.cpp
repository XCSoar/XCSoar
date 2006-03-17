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
#if (NEWINFOBOX>0)


#include "stdafx.h"
#include <Aygshell.h>

#include "XCSoar.h"

#include "WindowControls.h"
#include "Externs.h"
#include "McReady.h"
#include "dlgTools.h"
#include "InfoBoxLayout.h"
#include "MapWindow.h"


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

  if ((DrawListIndex < NUMAIRSPACECOLORS) &&(DrawListIndex>=0)) {
    int i = DrawListIndex;
    SelectObject(hDC, GetStockObject(WHITE_BRUSH));
    SelectObject(hDC, GetStockObject(BLACK_PEN));
    SetBkColor(hDC, 
	       RGB(0xFF, 0xFF, 0xFF));
    SelectObject(hDC, 
		 MapWindow::hAirspaceBrushes[1]); // this is the solid brush
    SetTextColor(hDC, 
		 MapWindow::Colours[i]);
    Rectangle(hDC, 
              100*InfoBoxLayout::scale, 
              2*InfoBoxLayout::scale,
              180*InfoBoxLayout::scale,
              22*InfoBoxLayout::scale);
  }
}


static void OnAirspaceColoursListEnter(WindowControl * Sender, 
				WndListFrame::ListInfo_t *ListInfo) {

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
  if (ListInfo->DrawIndex == -1){
    ListInfo->ItemCount = NUMAIRSPACECOLORS;
  } else {
    DrawListIndex = ListInfo->DrawIndex+ListInfo->ScrollIndex;
    ItemIndex = ListInfo->ItemIndex+ListInfo->ScrollIndex;
  }
}

static void OnCloseClicked(WindowControl * Sender){
  ItemIndex = -1;
  wf->SetModalResult(mrOK);
}


static CallBackTableEntry_t CallBackTable[]={
  DeclearCallBackEntry(OnAirspaceColoursPaintListItem),
  DeclearCallBackEntry(OnAirspaceColoursListInfo),
  DeclearCallBackEntry(OnCloseClicked),
  DeclearCallBackEntry(NULL)
};


int dlgAirspaceColoursShowModal(void){

  ItemIndex = -1;

  wf = dlgLoadFromXML(CallBackTable, "\\NOR Flash\\dlgAirspaceColours.xml", 
		      hWndMainWindow,
		      TEXT("IDR_XML_AIRSPACECOLOURS"));

  if (!wf) return -1;

  ASSERT(wf!=NULL);

  wAirspaceColoursList = (WndListFrame*)wf->FindByName(TEXT("frmAirspaceColoursList"));
  ASSERT(wAirspaceColoursList!=NULL);
  wAirspaceColoursList->SetBorderKind(BORDERLEFT);
  wAirspaceColoursList->SetEnterCallback(OnAirspaceColoursListEnter);

  wAirspaceColoursListEntry = (WndOwnerDrawFrame*)wf->
    FindByName(TEXT("frmAirspaceColoursListEntry"));
  ASSERT(wAirspaceColoursListEntry!=NULL);
  wAirspaceColoursListEntry->SetCanFocus(true);

  UpdateList();

  wf->ShowModal();

  // now retrieve back the properties...

  delete wf;

  wf = NULL;

  return ItemIndex;
}

#endif

