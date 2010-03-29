/*
Copyright_License {

  XCSoar Glide Computer - http://www.xcsoar.org/
  Copyright (C) 2000, 2001, 2002, 2003, 2004, 2005, 2006, 2007, 2008, 2009

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
	Tobias Bieniek <tobias.bieniek@gmx.de>

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
#include "Blackboard.hpp"
#include "SettingsTask.hpp"
#include "Logger.hpp"
#include "Math/FastMath.h"
#include "MainWindow.hpp"
#include "LocalPath.hpp"
#include "DataField/FileReader.hpp"
#include "StringUtil.hpp"
#include "Dialogs/dlgTaskHelpers.hpp"

#include "RenderTask.hpp"
#include "RenderTaskPoint.hpp"
#include "RenderObservationZone.hpp"
#include "Screen/Chart.hpp"
#include "ChartProjection.hpp"

#include <assert.h>

static SingleWindow *parent_window;
static WndForm *wf=NULL;
static WndListFrame* wTasks= NULL;
static WndFrame* wTaskView = NULL;
static OrderedTask* ordered_task= NULL;

static void OnCloseClicked(WindowControl * Sender)
{
  (void)Sender;
  wf->SetModalResult(mrCancel);
}

static void
OnTaskPaint(WindowControl *Sender, Canvas &canvas)
{
  RECT rc = Sender->get_client_rect();
  Chart chart(canvas, rc);

  if (wTasks->GetCursorIndex()>0) {
    chart.DrawNoData();
    return;
  }

  if (!ordered_task->check_task()) {
    chart.DrawNoData();
    return;
  }

  BufferCanvas buffer;
  BufferCanvas stencil;

  buffer.set(canvas);
  stencil.set(canvas);

  ChartProjection proj(rc, *ordered_task, XCSoarInterface::Basic().Location);

  MapDrawHelper helper(canvas, buffer, stencil, proj, rc,
                       XCSoarInterface::SettingsMap());
  RenderObservationZone ozv(helper);
  RenderTaskPoint tpv(helper, ozv, false, XCSoarInterface::Basic().Location);
  ::RenderTask dv(tpv);
  ordered_task->CAccept(dv); 
}

static CallBackTableEntry_t CallBackTable[]={
  DeclareCallBackEntry(OnCloseClicked),
  DeclareCallBackEntry(OnTaskPaint),
  DeclareCallBackEntry(NULL)
};

static unsigned UpLimit=0;


static void
OnTaskPaintListItem(Canvas &canvas, const RECT rc, unsigned DrawListIndex)
{
  TCHAR sTmp[120];
  if (DrawListIndex < UpLimit) {
    _stprintf(sTmp, _T(" File %d: %s"), DrawListIndex, gettext(_T("blah")));
    canvas.text(rc.left + Layout::FastScale(2), rc.top + Layout::FastScale(2),
                sTmp);
  }
}

static void
RefreshView()
{
  wTaskView->invalidate();

  WndFrame* wSummary = (WndFrame *)wf->FindByName(_T("frmSummary"));
  if (wSummary) {
    if (wTasks->GetCursorIndex()==0) {
      TCHAR text[300];
      OrderedTaskSummary(ordered_task, text);
      wSummary->SetCaption(text);
    } else {
      wSummary->SetCaption(_T("null"));
    }
  }
}


static void
OnTaskListEnter(unsigned ItemIndex)
{
  if (MessageBoxX(gettext(_T("Activate task?")),
                  gettext(_T("Task Selection")),
                  MB_YESNO|MB_ICONQUESTION) == IDYES) {

    /*
    delete ordered_task;
    ordered_task = foo->clone();
    RefreshView();
    */
  }
}

static void
OnTaskCursorCallback(unsigned i)
{
  RefreshView();
}

bool
dlgTaskListShowModal(SingleWindow &parent, OrderedTask** task)
{
  parent_window = &parent;

  ordered_task = *task;

  wf = NULL;

  if (!Layout::landscape) {
    wf = dlgLoadFromXML(CallBackTable,
                        _T("dlgTaskList_L.xml"),
                        parent,
                        _T("IDR_XML_TASKLIST_L"));
  } else {
    wf = dlgLoadFromXML(CallBackTable,
                        _T("dlgTaskList.xml"),
                        parent,
                        _T("IDR_XML_TASKLIST"));
  }

  if (!wf) return false;
  assert(wf!=NULL);

  wTaskView = (WndFrame*)wf->FindByName(_T("frmTaskView"));
  assert(wTaskView!=NULL);

  wTasks = (WndListFrame*)wf->FindByName(_T("frmTasks"));
  assert(wTasks!=NULL);
  wTasks->SetActivateCallback(OnTaskListEnter);
  wTasks->SetPaintItemCallback(OnTaskPaintListItem);
  wTasks->SetCursorCallback(OnTaskCursorCallback);
  UpLimit = 3;
  wTasks->SetLength(UpLimit);

  RefreshView();

  wf->ShowModal();
  delete wf;
  wf = NULL;

  if (*task != ordered_task) {
    *task = ordered_task;
    return true;
  } else {
    return false;
  }
}
