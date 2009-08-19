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

#include "externs.h"
#include "dlgTools.h"
#include "InfoBoxLayout.h"
#include "MapWindow.h"
#include "Registry.hpp"
#include "Screen/Util.hpp"

#include <assert.h>

static WndForm *wf=NULL;
static WndListFrame *wAirspaceList=NULL;
static WndOwnerDrawFrame *wAirspaceListEntry = NULL;

static int ItemIndex = -1;
static bool colormode = false;

int dlgAirspaceColoursShowModal(void);
int dlgAirspacePatternsShowModal(void);

static void UpdateList(void){
  wAirspaceList->ResetList();
  wAirspaceList->Redraw();
}

static int DrawListIndex=0;

static void OnAirspacePaintListItem(WindowControl * Sender, HDC hDC){

  TCHAR label[40];
  (void)Sender;
  if (DrawListIndex < AIRSPACECLASSCOUNT){
    int i = DrawListIndex;
    switch (i) {
    case CLASSA:
      _tcscpy(label, gettext(TEXT("Class A")));
      break;
    case CLASSB:
      _tcscpy(label, gettext(TEXT("Class B")));
      break;
    case CLASSC:
      _tcscpy(label, gettext(TEXT("Class C")));
      break;
    case CLASSD:
      _tcscpy(label, gettext(TEXT("Class D")));
      break;
    case CLASSE:
      _tcscpy(label, gettext(TEXT("Class E")));
      break;
    case CLASSF:
      _tcscpy(label, gettext(TEXT("Class F")));
      break;
    case PROHIBITED:
      _tcscpy(label, gettext(TEXT("Prohibited areas")));
      break;
    case DANGER:
      _tcscpy(label, gettext(TEXT("Danger areas")));
      break;
    case RESTRICT:
      _tcscpy(label, gettext(TEXT("Restricted areas")));
      break;
    case CTR:
      _tcscpy(label, gettext(TEXT("CTR")));
      break;
    case NOGLIDER:
      _tcscpy(label, gettext(TEXT("No gliders")));
      break;
    case WAVE:
      _tcscpy(label, gettext(TEXT("Wave")));
      break;
    case OTHER:
      _tcscpy(label, gettext(TEXT("Other")));
      break;
    case AATASK:
      _tcscpy(label, gettext(TEXT("AAT")));
      break;
    };

    int w0, w1, w2, x0;
    if (InfoBoxLayout::landscape) {
      w0 = 202*InfoBoxLayout::scale;
    } else {
      w0 = 225*InfoBoxLayout::scale;
    }
    w1 = GetTextWidth(hDC, gettext(TEXT("Warn")))+InfoBoxLayout::scale*10;
    w2 = GetTextWidth(hDC, gettext(TEXT("Display")))+InfoBoxLayout::scale*10;
    x0 = w0-w1-w2;

    ExtTextOutClip(hDC, 2*InfoBoxLayout::scale, 2*InfoBoxLayout::scale,
                   label, x0-InfoBoxLayout::scale*10);

    if (colormode) {

      SelectObject(hDC, GetStockObject(WHITE_PEN));
      SelectObject(hDC, GetStockObject(WHITE_BRUSH));
      Rectangle(hDC,
          x0, 2*InfoBoxLayout::scale,
          w0, 22*InfoBoxLayout::scale);
      SetTextColor(hDC,
         MapWindow::GetAirspaceColourByClass(i));
         SetBkColor(hDC,
         RGB(0xFF, 0xFF, 0xFF));
      SelectObject(hDC,
		   MapWindow::GetAirspaceBrushByClass(i));
        Rectangle(hDC,
        x0, 2*InfoBoxLayout::scale,
        w0, 22*InfoBoxLayout::scale);

    } else {

      bool iswarn;
      bool isdisplay;

      iswarn = (MapWindow::iAirspaceMode[i]>=2);
      isdisplay = ((MapWindow::iAirspaceMode[i]%2)>0);
      if (iswarn) {
        _tcscpy(label, gettext(TEXT("Warn")));
        ExtTextOut(hDC,
                   w0-w1-w2,
                   2*InfoBoxLayout::scale,
                   ETO_OPAQUE, NULL,
                   label,
                   _tcslen(label),
                   NULL);
      }
      if (isdisplay) {
        _tcscpy(label, gettext(TEXT("Display")));
        ExtTextOut(hDC,
                   w0-w2,
                   2*InfoBoxLayout::scale,
                   ETO_OPAQUE, NULL,
                   label,
                   _tcslen(label),
                   NULL);
      }

    }

  }
}


static bool changed = false;

static void OnAirspaceListEnter(WindowControl * Sender,
				WndListFrame::ListInfo_t *ListInfo) {
  (void)Sender;
  ItemIndex = ListInfo->ItemIndex + ListInfo->ScrollIndex;
  if (ItemIndex>=AIRSPACECLASSCOUNT) {
    ItemIndex = AIRSPACECLASSCOUNT-1;
  }
  if (ItemIndex>=0) {

    if (colormode) {
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
    } else {
      int v = (MapWindow::iAirspaceMode[ItemIndex]+1)%4;
      MapWindow::iAirspaceMode[ItemIndex] = v;
      //  wAirspaceList->Redraw();
      SetRegistryAirspaceMode(ItemIndex);
      changed = true;
    }
  }
}


static void OnAirspaceListInfo(WindowControl * Sender,
			       WndListFrame::ListInfo_t *ListInfo){
  (void)Sender;
  if (ListInfo->DrawIndex == -1){
    ListInfo->ItemCount = AIRSPACECLASSCOUNT;
  } else {
    DrawListIndex = ListInfo->DrawIndex+ListInfo->ScrollIndex;
    ItemIndex = ListInfo->ItemIndex+ListInfo->ScrollIndex;
  }
}

static void OnCloseClicked(WindowControl * Sender){
    (void)Sender;
	wf->SetModalResult(mrOK);
}


static void OnLookupClicked(WindowControl * Sender){
  (void)Sender;
  dlgAirspaceSelect();
}


static CallBackTableEntry_t CallBackTable[]={
  DeclareCallBackEntry(OnAirspacePaintListItem),
  DeclareCallBackEntry(OnAirspaceListInfo),
  DeclareCallBackEntry(OnCloseClicked),
  DeclareCallBackEntry(OnLookupClicked),
  DeclareCallBackEntry(NULL)
};


void dlgAirspaceShowModal(bool coloredit){

  colormode = coloredit;

  ItemIndex = -1;

  if (!InfoBoxLayout::landscape) {
    char filename[MAX_PATH];
    LocalPathS(filename, TEXT("dlgAirspace_L.xml"));
    wf = dlgLoadFromXML(CallBackTable,
                        filename,
                        hWndMainWindow,
                        TEXT("IDR_XML_AIRSPACE_L"));
  } else {
    char filename[MAX_PATH];
    LocalPathS(filename, TEXT("dlgAirspace.xml"));
    wf = dlgLoadFromXML(CallBackTable,
                        filename,
                        hWndMainWindow,
                        TEXT("IDR_XML_AIRSPACE"));
  }
  if (!wf) return;

  assert(wf!=NULL);

  wAirspaceList = (WndListFrame*)wf->FindByName(TEXT("frmAirspaceList"));
  assert(wAirspaceList!=NULL);
  wAirspaceList->SetBorderKind(BORDERLEFT);
  wAirspaceList->SetEnterCallback(OnAirspaceListEnter);

  wAirspaceListEntry = (WndOwnerDrawFrame*)wf->
    FindByName(TEXT("frmAirspaceListEntry"));
  assert(wAirspaceListEntry!=NULL);
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

