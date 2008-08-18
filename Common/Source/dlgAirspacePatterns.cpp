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


#include "StdAfx.h"
#include <aygshell.h>

#include "XCSoar.h"

#include "WindowControls.h"
#include "externs.h"
#include "McReady.h"
#include "dlgTools.h"
#include "InfoBoxLayout.h"
#include "MapWindow.h"


static WndForm *wf=NULL;
static WndListFrame *wAirspacePatternsList=NULL;
static WndOwnerDrawFrame *wAirspacePatternsListEntry = NULL;

static int ItemIndex = -1;


static void UpdateList(void){
  wAirspacePatternsList->ResetList();
  wAirspacePatternsList->Redraw();
}

static int DrawListIndex=0;

static void OnAirspacePatternsPaintListItem(WindowControl * Sender, HDC hDC){
  (void)Sender;
  if ((DrawListIndex < NUMAIRSPACEBRUSHES) &&(DrawListIndex>=0)) {
    int i = DrawListIndex;
    SelectObject(hDC, GetStockObject(WHITE_BRUSH));
    SelectObject(hDC, GetStockObject(BLACK_PEN));
    SetBkColor(hDC,
	       RGB(0xFF, 0xFF, 0xFF));
    SelectObject(hDC,
		 MapWindow::hAirspaceBrushes[i]);
    SetTextColor(hDC, RGB(0x00,0x00, 0x00));
    Rectangle(hDC,
              100*InfoBoxLayout::scale,
              2*InfoBoxLayout::scale,
              180*InfoBoxLayout::scale,
              22*InfoBoxLayout::scale);
  }
}


static void OnAirspacePatternsListEnter(WindowControl * Sender,
				WndListFrame::ListInfo_t *ListInfo) {
  (void)Sender;
  ItemIndex = ListInfo->ItemIndex + ListInfo->ScrollIndex;
  if (ItemIndex>=NUMAIRSPACEBRUSHES) {
    ItemIndex = NUMAIRSPACEBRUSHES-1;
  }
  if (ItemIndex>=0) {
    wf->SetModalResult(mrOK);
  }
}


static void OnAirspacePatternsListInfo(WindowControl * Sender,
			       WndListFrame::ListInfo_t *ListInfo){
  (void)Sender;
  if (ListInfo->DrawIndex == -1){
    ListInfo->ItemCount = NUMAIRSPACEBRUSHES;
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
  DeclearCallBackEntry(OnAirspacePatternsPaintListItem),
  DeclearCallBackEntry(OnAirspacePatternsListInfo),
  DeclearCallBackEntry(OnCloseClicked),
  DeclearCallBackEntry(NULL)
};


int dlgAirspacePatternsShowModal(void){

  ItemIndex = -1;

  if (!InfoBoxLayout::landscape) {
    char filename[MAX_PATH];
    LocalPathS(filename, TEXT("dlgAirspacePatterns_L.xml"));
    wf = dlgLoadFromXML(CallBackTable,

                        filename,
                        hWndMainWindow,
                        TEXT("IDR_XML_AIRSPACEPATTERNS_L"));
  } else {
    char filename[MAX_PATH];
    LocalPathS(filename, TEXT("dlgAirspacePatterns.xml"));
    wf = dlgLoadFromXML(CallBackTable,
                        filename,
                        hWndMainWindow,
                        TEXT("IDR_XML_AIRSPACEPATTERNS"));
  }

  if (!wf) return -1;

  ASSERT(wf!=NULL);

  wAirspacePatternsList = (WndListFrame*)wf->FindByName(TEXT("frmAirspacePatternsList"));
  ASSERT(wAirspacePatternsList!=NULL);
  wAirspacePatternsList->SetBorderKind(BORDERLEFT);
  wAirspacePatternsList->SetEnterCallback(OnAirspacePatternsListEnter);

  wAirspacePatternsListEntry = (WndOwnerDrawFrame*)wf->
    FindByName(TEXT("frmAirspacePatternsListEntry"));
  ASSERT(wAirspacePatternsListEntry!=NULL);
  wAirspacePatternsListEntry->SetCanFocus(true);

  UpdateList();

  wf->ShowModal();

  // now retrieve back the properties...

  delete wf;

  wf = NULL;

  return ItemIndex;
}

