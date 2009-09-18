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

#include "Dialogs/Internal.hpp"
#include "Protection.hpp"
#include "SettingsTask.hpp"
#include "InfoBoxLayout.h"
#include "MainWindow.hpp"
#include "WayPointList.hpp"
#include "Components.hpp"

#include <assert.h>

static WndForm *wf=NULL;
static WndListFrame *wStartPointList=NULL;
static WndOwnerDrawFrame *wStartPointListEntry = NULL;

static int ItemIndex = -1;

static void UpdateList(void){
  wStartPointList->ResetList();
  wStartPointList->Redraw();
}

static int DrawListIndex=0;

static void
OnStartPointPaintListItem(WindowControl * Sender, Canvas &canvas)
{
	(void)Sender;

  TCHAR label[MAX_PATH];

  if (DrawListIndex < MAXSTARTPOINTS){
    int i = DrawListIndex;

    if ((task_start_points[i].Index != -1)
	&&(task_start_stats[i].Active)) {
      _tcscpy(label, way_points.get(task_start_points[i].Index).Name);
    } else {
      int j;
      int i0=0;
      for (j=MAXSTARTPOINTS-1; j>=0; j--) {
        if ((task_start_points[j].Index!= -1)&&(task_start_stats[j].Active)) {
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

    canvas.text_opaque(2 * InfoBoxLayout::scale, 2 * InfoBoxLayout::scale,
                       label);
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
    res = dlgWayPointSelect(XCSoarInterface::Basic().Location);    
    if (res>=0) {
      task.SetStartPoint(ItemIndex, res);
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
  task.ClearStartPoints();
  changed = true;
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
    wf = dlgLoadFromXML(CallBackTable,
                        TEXT("dlgStartPoint_L.xml"),
                        XCSoarInterface::main_window,
                        TEXT("IDR_XML_STARTPOINT_L"));
  } else {
    wf = dlgLoadFromXML(CallBackTable,
                        TEXT("dlgStartPoint.xml"),
                        XCSoarInterface::main_window,
                        TEXT("IDR_XML_STARTPOINT"));
  }
  if (!wf) return;

  assert(wf!=NULL);

  task.CheckStartPointInTask();

  wStartPointList = (WndListFrame*)wf->FindByName(TEXT("frmStartPointList"));
  assert(wStartPointList!=NULL);
  wStartPointList->SetBorderKind(BORDERLEFT);
  wStartPointList->SetEnterCallback(OnStartPointListEnter);

  wStartPointListEntry = (WndOwnerDrawFrame*)wf->
    FindByName(TEXT("frmStartPointListEntry"));
  assert(wStartPointListEntry!=NULL);
  wStartPointListEntry->SetCanFocus(true);

  UpdateList();

  changed = false;

  wf->ShowModal();

  // now retrieve back the properties...
  if (changed) {
    task.SetTaskModified();
    task.RefreshTask(XCSoarInterface::SettingsComputer());
  };

  delete wf;

  wf = NULL;

}

