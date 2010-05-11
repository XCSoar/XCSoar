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
#include "DataField/FileReader.hpp"
#include "StringUtil.hpp"
#include "Dialogs/dlgTaskHelpers.hpp"

#include "RenderTask.hpp"
#include "RenderTaskPoint.hpp"
#include "RenderObservationZone.hpp"
#include "Screen/Chart.hpp"
#include "ChartProjection.hpp"
#include "TaskStore.hpp"
#include "TaskClientUI.hpp"
#include "Components.hpp"

#include <assert.h>

static SingleWindow *parent_window;
static WndForm *wf=NULL;
static WndListFrame* wTasks= NULL;
static WndFrame* wTaskView = NULL;
static TaskStore task_store;
static OrderedTask* active_task = NULL;
static bool task_modified;

static void
OnCloseClicked(WindowControl * Sender)
{
  (void)Sender;
  wf->SetModalResult(mrCancel);
}

static unsigned
get_cursor_index()
{
  return wTasks->GetCursorIndex();
}

static bool
cursor_at_active_task()
{
  return (wTasks->GetCursorIndex() == 0);
}

static OrderedTask*
get_cursor_task()
{
  if (cursor_at_active_task())
    return active_task;

  if (get_cursor_index() > task_store.size())
    return NULL;

  return task_store.get_task(get_cursor_index() - 1);
}

static tstring
get_cursor_name()
{
  if (cursor_at_active_task())
    return _T("(Active Task)");

  if (get_cursor_index() > task_store.size())
    return _T("");

  return task_store.get_name(get_cursor_index() - 1);
}

static void
OnTaskPaint(WindowControl *Sender, Canvas &canvas)
{
  RECT rc = Sender->get_client_rect();
  Chart chart(canvas, rc);

  OrderedTask* ordered_task = get_cursor_task();
  if (ordered_task == NULL) {
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

static void
OnTaskPaintListItem(Canvas &canvas, const RECT rc, unsigned DrawListIndex)
{
  if (DrawListIndex > task_store.size())
    return;

  tstring name;
  if (DrawListIndex == 0)
    name = _T("(Active task)");
  else
    name = task_store.get_name(DrawListIndex-1);

  canvas.text(rc.left + Layout::FastScale(2),
              rc.top + Layout::FastScale(2),
              name.c_str());
}

static void
RefreshView()
{
  wTasks->SetLength(task_store.size() + 1);
  wTaskView->invalidate();

  WndFrame* wSummary = (WndFrame *)wf->FindByName(_T("frmSummary"));
  if (!wSummary)
    return;

  OrderedTask* ordered_task = get_cursor_task();
  if (ordered_task == NULL) {
    wSummary->SetCaption(_T(""));
    return;
  }

  TCHAR text[300];
  OrderedTaskSummary(ordered_task, text);
  wSummary->SetCaption(text);
}

static void
OnSave()
{
  if (!cursor_at_active_task())
    return;

  if (!OrderedTaskSave(*active_task))
    return;

  task_store.scan();
  RefreshView();
}

static void
OnLoad()
{
  if (cursor_at_active_task())
    return;

  const OrderedTask* orig = get_cursor_task();
  if (orig == NULL)
    return;

  (void)get_cursor_name();

  if (MessageBoxX(gettext(_T("Load the selected task?")),
                  gettext(_T("Task Browser")),
                  MB_YESNO | MB_ICONQUESTION) != IDYES)
    return;

  delete active_task;
  active_task = task_ui.task_copy(*orig);
  RefreshView();
  task_modified = true;
}

static void
OnLoadSave()
{
  if (cursor_at_active_task())
    OnSave();
  else
    OnLoad();
}

static void
UpdateButtons()
{
  WndButton* wbSelect = (WndButton*)wf->FindByName(_T("cmdLoadSave"));
  if (!wbSelect)
    return;

  if (cursor_at_active_task()) {
    wbSelect->SetCaption(_T("Save"));
    return;
  }

  wbSelect->SetCaption(_T("Load"));
}

static void 
OnLoadSaveClicked(WindowControl * Sender)
{
  OnLoadSave();
}

static void
OnTaskListEnter(unsigned ItemIndex)
{
  OnLoadSave();
}

static void
OnTaskCursorCallback(unsigned i)
{
  UpdateButtons();
  RefreshView();
}

static CallBackTableEntry_t CallBackTable[] = {
  DeclareCallBackEntry(OnCloseClicked),
  DeclareCallBackEntry(OnLoadSaveClicked),
  DeclareCallBackEntry(OnTaskPaint),
  DeclareCallBackEntry(NULL)
};

bool
dlgTaskListShowModal(SingleWindow &parent, OrderedTask** task)
{
  parent_window = &parent;
  
  active_task = *task;
  task_modified = false;

  task_store.scan();

  if (Layout::landscape)
    wf = dlgLoadFromXML(CallBackTable, _T("dlgTaskList_L.xml"),
                        parent, _T("IDR_XML_TASKLIST_L"));
  else
    wf = dlgLoadFromXML(CallBackTable, _T("dlgTaskList.xml"),
                        parent, _T("IDR_XML_TASKLIST"));

  if (!wf)
    return false;

  assert(wf != NULL);

  wTaskView = (WndFrame*)wf->FindByName(_T("frmTaskView"));
  assert(wTaskView != NULL);

  wTasks = (WndListFrame*)wf->FindByName(_T("frmTasks"));
  assert(wTasks != NULL);
  wTasks->SetActivateCallback(OnTaskListEnter);
  wTasks->SetPaintItemCallback(OnTaskPaintListItem);
  wTasks->SetCursorCallback(OnTaskCursorCallback);

  UpdateButtons();
  RefreshView();

  wf->ShowModal();
  delete wf;
  
  bool retval = false;
  if (*task != active_task) {
    *task = active_task;
    retval = true;
  }

  task_store.clear();

  return retval || task_modified;
}
