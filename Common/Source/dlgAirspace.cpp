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
static WndListFrame *wAirspaceList=NULL;
static WndOwnerDrawFrame *wAirspaceListEntry = NULL;

static int ItemIndex = -1;

int dlgAirspaceColoursShowModal(void);
int dlgAirspacePatternsShowModal(void);

static void UpdateList(void){
  wAirspaceList->ResetList();
  wAirspaceList->Redraw();
}

static int DrawListIndex=0;

static void OnAirspacePaintListItem(WindowControl * Sender, HDC hDC){

  TCHAR label[40];

  if (DrawListIndex < AIRSPACECLASSCOUNT){
    int i = DrawListIndex;
    switch (i) {
    case CLASSA: 
      _tcscpy(label, TEXT("Class A"));
      break;
    case CLASSB: 
      _tcscpy(label, TEXT("Class B"));
      break;
    case CLASSC: 
      _tcscpy(label, TEXT("Class C"));
      break;
    case CLASSD: 
      _tcscpy(label, TEXT("Class D"));
      break;
    case CLASSE: 
      _tcscpy(label, TEXT("Class E"));
      break;
    case CLASSF: 
      _tcscpy(label, TEXT("Class F"));
      break;
    case PROHIBITED: 
      _tcscpy(label, TEXT("Prohibited areas"));
      break;
    case DANGER: 
      _tcscpy(label, TEXT("Danger areas"));
      break;
    case RESTRICT: 
      _tcscpy(label, TEXT("Restricted areas"));
      break;
    case CTR: 
      _tcscpy(label, TEXT("CTR"));
      break;
    case NOGLIDER: 
      _tcscpy(label, TEXT("No gliders"));
      break;
    case WAVE: 
      _tcscpy(label, TEXT("Wave"));
      break;
    case OTHER: 
      _tcscpy(label, TEXT("Other"));
      break;
    case AATASK: 
      _tcscpy(label, TEXT("AAT"));
      break;
    };
    ExtTextOut(hDC, 
	       2*InfoBoxLayout::scale, 
	       2*InfoBoxLayout::scale,
	       ETO_OPAQUE, NULL,
	       label,
	       _tcslen(label), 
	       NULL);
    SelectObject(hDC, GetStockObject(WHITE_PEN));
    SelectObject(hDC, GetStockObject(WHITE_BRUSH));
    Rectangle(hDC, 
              100*InfoBoxLayout::scale, 
              2*InfoBoxLayout::scale,
              180*InfoBoxLayout::scale,
              22*InfoBoxLayout::scale);
    SetTextColor(hDC, 
		 MapWindow::Colours[MapWindow::iAirspaceColour[i]]);
    SetBkColor(hDC, 
	       RGB(0xFF, 0xFF, 0xFF));
    SelectObject(hDC, 
		 MapWindow::hAirspaceBrushes[MapWindow::iAirspaceBrush[i]]);
    Rectangle(hDC, 
              100*InfoBoxLayout::scale, 
              2*InfoBoxLayout::scale,
              180*InfoBoxLayout::scale,
              22*InfoBoxLayout::scale);

  }
}


static bool changed = false;

static void OnAirspaceListEnter(WindowControl * Sender, 
				WndListFrame::ListInfo_t *ListInfo) {

  ItemIndex = ListInfo->ItemIndex + ListInfo->ScrollIndex;
  if (ItemIndex>=AIRSPACECLASSCOUNT) {
    ItemIndex = AIRSPACECLASSCOUNT-1;
  }
  if (ItemIndex>=0) {
    int c = dlgAirspaceColoursShowModal();
    if (c>=0) {
      MapWindow::iAirspaceColour[ItemIndex] = c; 
      SetRegistryColour(ItemIndex,MapWindow::iAirspaceColour[ItemIndex]);
      changed = true;
    }
    int p = dlgAirspacePatternsShowModal();
    if (p>=0) {
      MapWindow::iAirspaceBrush[ItemIndex] = p; 
      SetRegistryBrush(ItemIndex,MapWindow::iAirspaceBrush[ItemIndex]);
      changed = true;
    }
  }
}


static void OnAirspaceListInfo(WindowControl * Sender, 
			       WndListFrame::ListInfo_t *ListInfo){
  if (ListInfo->DrawIndex == -1){
    ListInfo->ItemCount = AIRSPACECLASSCOUNT;
  } else {
    DrawListIndex = ListInfo->DrawIndex+ListInfo->ScrollIndex;
    ItemIndex = ListInfo->ItemIndex+ListInfo->ScrollIndex;
  }
}

static void OnCloseClicked(WindowControl * Sender){
  wf->SetModalResult(mrOK);
}


static CallBackTableEntry_t CallBackTable[]={
  DeclearCallBackEntry(OnAirspacePaintListItem),
  DeclearCallBackEntry(OnAirspaceListInfo),
  DeclearCallBackEntry(OnCloseClicked),
  DeclearCallBackEntry(NULL)
};


void dlgAirspaceShowModal(void){

  ItemIndex = -1;

  wf = dlgLoadFromXML(CallBackTable, "\\NOR Flash\\dlgAirspace.xml", 
		      hWndMainWindow);

  if (!wf) return;

  ASSERT(wf!=NULL);

  wAirspaceList = (WndListFrame*)wf->FindByName(TEXT("frmAirspaceList"));
  ASSERT(wAirspaceList!=NULL);
  wAirspaceList->SetBorderKind(BORDERLEFT);
  wAirspaceList->SetEnterCallback(OnAirspaceListEnter);

  wAirspaceListEntry = (WndOwnerDrawFrame*)wf->
    FindByName(TEXT("frmAirspaceListEntry"));
  ASSERT(wAirspaceListEntry!=NULL);
  wAirspaceListEntry->SetCanFocus(true);

  UpdateList();

  changed = false;

  wf->ShowModal();

  // now retrieve back the properties...
  if (changed) {
    StoreRegistry();
    DoStatusMessage(TEXT("Configuration saved"));
  };

  delete wf;

  wf = NULL;

}

#endif
