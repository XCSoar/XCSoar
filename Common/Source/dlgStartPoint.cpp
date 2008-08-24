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
static WndListFrame *wStartPointList=NULL;
static WndOwnerDrawFrame *wStartPointListEntry = NULL;

static int ItemIndex = -1;

static void UpdateList(void){
  wStartPointList->ResetList();
  wStartPointList->Redraw();
}

static int DrawListIndex=0;

static void OnStartPointPaintListItem(WindowControl * Sender, HDC hDC){
	(void)Sender;

  TCHAR label[MAX_PATH];

  if (DrawListIndex < MAXSTARTPOINTS){
    int i = DrawListIndex;

    if ((StartPoints[i].Index != -1)&&(StartPoints[i].Active)) {
      _tcscpy(label, WayPointList[StartPoints[i].Index].Name);
    } else {
      int j;
      int i0=0;
      for (j=MAXSTARTPOINTS-1; j>=0; j--) {
        if ((StartPoints[j].Index!= -1)&&(StartPoints[j].Active)) {
          i0=j+1;
          break;
        }
      }
      if (i==i0) {
        _tcscpy(label, TEXT("(add waypoint)"));
      } else {
        _tcscpy(label, TEXT(" "));
      }
    }
    ExtTextOut(hDC,
	       2*InfoBoxLayout::scale,
	       2*InfoBoxLayout::scale,
	       ETO_OPAQUE, NULL,
	       label,
	       _tcslen(label),
	       NULL);
  }
}


static bool changed = false;

static void OnStartPointListEnter(WindowControl * Sender,
				WndListFrame::ListInfo_t *ListInfo) {
  (void)Sender;
  ItemIndex = ListInfo->ItemIndex + ListInfo->ScrollIndex;
  if (ItemIndex>=MAXSTARTPOINTS) {
    ItemIndex = MAXSTARTPOINTS-1;
  }
  if (ItemIndex>=0) {
    int res;
    res = dlgWayPointSelect();
    if (res>=0) {
      LockTaskData();
      StartPoints[ItemIndex].Index = res;
      StartPoints[ItemIndex].Active = true;
      UnlockTaskData();
      changed = true;
    }
  }
}


static void OnStartPointListInfo(WindowControl * Sender,
			       WndListFrame::ListInfo_t *ListInfo){
	(void)Sender;
  if (ListInfo->DrawIndex == -1){
    ListInfo->ItemCount = MAXSTARTPOINTS;
  } else {
    DrawListIndex = ListInfo->DrawIndex+ListInfo->ScrollIndex;
    ItemIndex = ListInfo->ItemIndex+ListInfo->ScrollIndex;
  }
}

static void OnCloseClicked(WindowControl * Sender){
	(void)Sender;
  wf->SetModalResult(mrOK);
}

static void OnClearClicked(WindowControl * Sender){
	(void)Sender;
  LockTaskData();
  for (int i=0; i<MAXSTARTPOINTS; i++) {
    StartPoints[i].Index = -1;
    StartPoints[i].Active = false;
  }
  StartPoints[0].Index = Task[0].Index;
  StartPoints[0].Active = true;
  changed = true;
  UnlockTaskData();
  UpdateList();
}


static CallBackTableEntry_t CallBackTable[]={
  DeclareCallBackEntry(OnStartPointPaintListItem),
  DeclareCallBackEntry(OnStartPointListInfo),
  DeclareCallBackEntry(OnCloseClicked),
  DeclareCallBackEntry(OnClearClicked),
  DeclareCallBackEntry(NULL)
};


void dlgStartPointShowModal(void) {

  ItemIndex = -1;

  if (!InfoBoxLayout::landscape) {
    char filename[MAX_PATH];
    LocalPathS(filename, TEXT("dlgStartPoint_L.xml"));
    wf = dlgLoadFromXML(CallBackTable,
                        filename,
                        hWndMainWindow,
                        TEXT("IDR_XML_STARTPOINT_L"));
  } else {
    char filename[MAX_PATH];
    LocalPathS(filename, TEXT("dlgStartPoint.xml"));
    wf = dlgLoadFromXML(CallBackTable,
                        filename,
                        hWndMainWindow,
                        TEXT("IDR_XML_STARTPOINT"));
  }
  if (!wf) return;

  ASSERT(wf!=NULL);

  wStartPointList = (WndListFrame*)wf->FindByName(TEXT("frmStartPointList"));
  ASSERT(wStartPointList!=NULL);
  wStartPointList->SetBorderKind(BORDERLEFT);
  wStartPointList->SetEnterCallback(OnStartPointListEnter);

  wStartPointListEntry = (WndOwnerDrawFrame*)wf->
    FindByName(TEXT("frmStartPointListEntry"));
  ASSERT(wStartPointListEntry!=NULL);
  wStartPointListEntry->SetCanFocus(true);

  UpdateList();

  changed = false;

  wf->ShowModal();

  // now retrieve back the properties...
  if (changed) {
    LockTaskData();
    TaskModified = true;
    RefreshTask();
    UnlockTaskData();
    /// TODO do something...
  };

  delete wf;

  wf = NULL;

}

