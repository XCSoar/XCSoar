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
#include "Logger/Logger.hpp"
#include "Math/FastMath.h"
#include "MainWindow.hpp"
#include "LocalPath.hpp"
#include "StringUtil.hpp"

#include "Dialogs/dlgTaskHelpers.hpp"

#include "Task/TaskPoints/StartPoint.hpp"
#include "Task/TaskPoints/FinishPoint.hpp"
#include "Task/Visitors/TaskVisitor.hpp"

#include "RenderTask.hpp"
#include "RenderTaskPoint.hpp"
#include "RenderObservationZone.hpp"
#include "Screen/Chart.hpp"
#include "ChartProjection.hpp"

#include <assert.h>

static SingleWindow *parent_window;
static WndForm *wf=NULL;
static WndFrame* wTaskView= NULL;
static WndListFrame* wTaskPoints= NULL;
static OrderedTask* ordered_task= NULL;
static bool task_modified = false;

static void OnCloseClicked(WindowControl * Sender)
{
  (void)Sender;
  wf->SetModalResult(mrCancel);
}

static void
RefreshView()
{
  if (!ordered_task->is_max_size()) {
    wTaskPoints->SetLength(ordered_task->task_size()+1);
  } else {
    wTaskPoints->SetLength(ordered_task->task_size());
  }
  wTaskView->invalidate();
  wTaskPoints->invalidate();

  WndFrame* wSummary = (WndFrame *)wf->FindByName(_T("frmSummary"));
  if (wSummary) {
    TCHAR text[300];
    OrderedTaskSummary(ordered_task, text);
    wSummary->SetCaption(text);
  }
}


static void OnPropertiesClicked(WindowControl * Sender)
{
  (void)Sender;
  task_modified |= dlgTaskPropertiesShowModal(*parent_window, &ordered_task);
  RefreshView();
}

static void OnNewClicked(WindowControl * Sender)
{
  (void)Sender;

  if ((ordered_task->task_size()<2) || 
      (MessageBoxX(gettext(_T("Clear task?")),
                   gettext(_T("Task edit")),
                   MB_YESNO|MB_ICONQUESTION) == IDYES)) {

    task_modified = true;
    ordered_task->clear();
    dlgTaskTypeShowModal(*parent_window, &ordered_task);
    RefreshView();
  }
}

static void
OnTaskPaint(WindowControl *Sender, Canvas &canvas)
{
  RECT rc = Sender->get_client_rect();

  Chart chart(canvas, rc);

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


static void
OnTaskPaintListItem(Canvas &canvas, const RECT rc, unsigned DrawListIndex)
{
  TCHAR sTmp[120];
  if (DrawListIndex > ordered_task->task_size()) {
    // error!
    return;
  }
  if (DrawListIndex == ordered_task->task_size()) {
    if (!ordered_task->is_max_size()) {
      _stprintf(sTmp, _T("  (%s)"), gettext(_T("add waypoint")));
      canvas.text(rc.left + Layout::FastScale(2), rc.top + Layout::FastScale(2),
                  sTmp);
    }
  } else {
    OrderedTaskPointLabel(ordered_task, DrawListIndex, sTmp);
    canvas.text(rc.left + Layout::FastScale(2), rc.top + Layout::FastScale(2),
                sTmp);
  }
}

static void
OnTaskListEnter(unsigned ItemIndex)
{
  if (ItemIndex< ordered_task->task_size()) {
    if (dlgTaskPointShowModal(*parent_window, &ordered_task, ItemIndex)) {
      task_modified = true;
      RefreshView();
    }
  } else if (!ordered_task->is_max_size()) {
    if (dlgTaskPointNew(*parent_window, &ordered_task, ItemIndex)) {
      task_modified = true;
      RefreshView();
    }
  }
}

static void
OnDeclareClicked(WindowControl * Sender)
{
  (void)Sender;
  logger.LoggerDeviceDeclare(*ordered_task);
}

static void
OnSaveClicked(WindowControl * Sender)
{
  (void)Sender;
  if (!ordered_task->check_task()) {
    MessageBoxX (gettext(TEXT("Task invalid.  Not saved.")),
                 TEXT("Task Edit"), MB_OK);
    return;
  }

  if (OrderedTaskSave(*ordered_task, true)) {
    MessageBoxX (gettext(TEXT("Task saved")),
                 TEXT("Task Edit"), MB_OK);
  }
}

static CallBackTableEntry_t CallBackTable[]={
  DeclareCallBackEntry(OnCloseClicked),
  DeclareCallBackEntry(OnPropertiesClicked),
  DeclareCallBackEntry(OnNewClicked),
  DeclareCallBackEntry(OnSaveClicked),
  DeclareCallBackEntry(OnTaskPaint),
  DeclareCallBackEntry(OnDeclareClicked),
  DeclareCallBackEntry(NULL)
};

bool
dlgTaskEditShowModal(SingleWindow &parent, OrderedTask** task)
{
  ordered_task = *task;
  parent_window = &parent;
  task_modified = false;

  wf = NULL;

  if (!Layout::landscape) {
    wf = dlgLoadFromXML(CallBackTable,
                        _T("dlgTaskEdit_L.xml"),
                        parent,
                        _T("IDR_XML_TASKEDIT_L"));
  } else {
    wf = dlgLoadFromXML(CallBackTable,
                        _T("dlgTaskEdit.xml"),
                        parent,
                        _T("IDR_XML_TASKEDIT"));
  }

  wTaskPoints = (WndListFrame*)wf->FindByName(_T("frmTaskPoints"));
  assert(wTaskPoints!=NULL);

  wTaskView = (WndFrame*)wf->FindByName(_T("frmTaskView"));
  assert(wTaskView!=NULL);

  wTaskPoints->SetActivateCallback(OnTaskListEnter);
  wTaskPoints->SetPaintItemCallback(OnTaskPaintListItem);

  RefreshView();

  if (!wf) return false;
  assert(wf!=NULL);
  wf->ShowModal();
  delete wf;
  wf = NULL;

  if (*task != ordered_task) {
    *task = ordered_task;
    return true;
  } else {
    return task_modified;
  }
}
