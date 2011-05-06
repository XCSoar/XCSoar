/*
Copyright_License {

  XCSoar Glide Computer - http://www.xcsoar.org/
  Copyright (C) 2000-2011 The XCSoar Project
  A detailed list of copyright holders can be found in the file "AUTHORS".

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
#include "Screen/Layout.hpp"
#include "Protection.hpp"
#include "SettingsTask.hpp"
#include "Task.h"
#include "MainWindow.hpp"
#include "WayPointList.hpp"
#include "Components.hpp"

#include <assert.h>

static WndForm *wf=NULL;
static WndListFrame *wStartPointList=NULL;

static void
OnStartPointPaintListItem(Canvas &canvas, const RECT rc, unsigned i)
{
  assert(i < MAXSTARTPOINTS);

  TCHAR label[MAX_PATH];

  if ((task_start_points[i].Index != -1)
      &&(task_start_stats[i].Active)) {
    _tcscpy(label, way_points.get(task_start_points[i].Index).Name);
  } else {
    int j;
    unsigned i0 = 0;
    for (j=MAXSTARTPOINTS-1; j>=0; j--) {
      if ((task_start_points[j].Index!= -1)&&(task_start_stats[j].Active)) {
        i0=j+1;
        break;
      }
    }
    if (i==i0) {
      _tcscpy(label, _("(Add Waypoint)"));
    } else {
      _tcscpy(label, _T(" "));
    }
  }

  canvas.text(rc.left + Layout::FastScale(2), rc.top + Layout::FastScale(2),
              label);
}


static bool changed = false;

static void
OnStartPointListEnter(unsigned ItemIndex) {
  if (ItemIndex>=MAXSTARTPOINTS) {
    ItemIndex = MAXSTARTPOINTS-1;
  }

  int res;
  res = dlgWayPointSelect(XCSoarInterface::Basic().Location);
  if (res>=0) {
    task.SetStartPoint(ItemIndex, res);
    changed = true;
  }
}

static void
OnCloseClicked(gcc_unused WndButton &Sender)
{
  wf->SetModalResult(mrOK);
}

static void
OnClearClicked(gcc_unused WndButton &Sender)
{
  task.ClearStartPoints();
  changed = true;

  wStartPointList->invalidate();
}


static CallBackTableEntry CallBackTable[]={
  DeclareCallBackEntry(OnCloseClicked),
  DeclareCallBackEntry(OnClearClicked),
  DeclareCallBackEntry(NULL)
};


void dlgStartPointShowModal(void) {
  if (!Layout::landscape) {
    wf = LoadDialog(CallBackTable,
                        XCSoarInterface::main_window,
                        _T("IDR_XML_STARTPOINT_L"));
  } else {
    wf = LoadDialog(CallBackTable,
                        XCSoarInterface::main_window,
                        _T("IDR_XML_STARTPOINT"));
  }
  if (!wf) return;

  assert(wf!=NULL);

  task.CheckStartPointInTask();

  wStartPointList = (WndListFrame*)wf->FindByName(_T("frmStartPointList"));
  assert(wStartPointList!=NULL);
  wStartPointList->SetBorderKind(BORDERLEFT);
  wStartPointList->SetActivateCallback(OnStartPointListEnter);
  wStartPointList->SetPaintItemCallback(OnStartPointPaintListItem);
  wStartPointList->SetLength(MAXSTARTPOINTS);

  changed = false;

  wf->ShowModal();

  // now retrieve back the properties...
  if (changed) {
    task.SetTaskModified();
    task.RefreshTask(XCSoarInterface::SettingsComputer(),
                     XCSoarInterface::Basic());
  };

  delete wf;

  wf = NULL;

}

