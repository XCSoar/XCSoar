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
#include "Task/ProtectedTaskManager.hpp"
#include "Components.hpp"
#include "LocalPath.hpp"

#include "BackgroundDrawHelper.hpp"

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

  ChartProjection proj(rc, *ordered_task, XCSoarInterface::Basic().Location);

  BackgroundDrawHelper background;
  background.set_terrain(&terrain);
  background.Draw(canvas, rc, proj, XCSoarInterface::SettingsMap());

  RenderObservationZone ozv(canvas, proj, XCSoarInterface::SettingsMap());
  RenderTaskPoint tpv(canvas, proj, XCSoarInterface::SettingsMap(),
                      ozv, false, XCSoarInterface::Basic().Location);
  ::RenderTask dv(tpv);
  dv.Visit(*ordered_task);
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

  tstring fname = get_cursor_name();
  tstring text = _("Load the selected task?");
  text += _T("\n(");
  text += fname;
  text += _T(")");

  if (MessageBoxX(text.c_str(), _("Task Browser"),
                  MB_YESNO | MB_ICONQUESTION) != IDYES)
    return;

  delete active_task;
  active_task = protected_task_manager.task_copy(*orig);
  RefreshView();
  task_modified = true;

  wf->SetModalResult(mrOK);
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
OnDelete()
{
  if (cursor_at_active_task())
    return;

  tstring fname = get_cursor_name();
  tstring text = _("Delete the selected task?");
  text += _T("\n(");
  text += fname;
  text += _T(")");

  if (MessageBoxX(text.c_str(), _("Task Browser"),
                  MB_YESNO | MB_ICONQUESTION) != IDYES)
    return;

  TCHAR path[MAX_PATH];
  LocalPath(path, fname.c_str());
  DeleteFile(path);

  task_store.scan();
  RefreshView();
}

static void
OnRename()
{
  if (cursor_at_active_task())
    return;

  tstring oldname = get_cursor_name();
  tstring newname = oldname;
  if (newname.find(_T(".tsk")) != tstring::npos)
    newname = newname.substr(0, newname.find(_T(".tsk")));

  if (!dlgTextEntryShowModal(newname, 10))
    return;

  newname += _T(".tsk");

  TCHAR oldpath[MAX_PATH];
  TCHAR newpath[MAX_PATH];
  LocalPath(oldpath, oldname.c_str());
  LocalPath(newpath, newname.c_str());

  MoveFile(oldpath, newpath);

  task_store.scan();
  RefreshView();
}

static void
UpdateButtons()
{
  WndButton* wbLoadSave = (WndButton*)wf->FindByName(_T("cmdLoadSave"));
  WndButton* wbDelete = (WndButton*)wf->FindByName(_T("cmdDelete"));
  WndButton* wbRename = (WndButton*)wf->FindByName(_T("cmdRename"));
  if (!wbLoadSave || !wbDelete || !wbRename)
    return;

  if (cursor_at_active_task()) {
    wbLoadSave->SetCaption(_T("Save"));
    wbDelete->set_enabled(false);
    wbDelete->SetForeColor(Color::GRAY);
    wbRename->set_enabled(false);
    wbRename->SetForeColor(Color::GRAY);
    return;
  }

  wbLoadSave->SetCaption(_T("Load"));
  wbDelete->set_enabled(true);
  wbDelete->SetForeColor(Color::BLACK);
  wbRename->set_enabled(true);
  wbRename->SetForeColor(Color::BLACK);
}

static void 
OnLoadSaveClicked(WindowControl * Sender)
{
  OnLoadSave();
}

static void
OnDeleteClicked(WindowControl * Sender)
{
  OnDelete();
}

static void
OnRenameClicked(WindowControl * Sender)
{
  OnRename();
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
  DeclareCallBackEntry(OnDeleteClicked),
  DeclareCallBackEntry(OnRenameClicked),
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
    wf = dlgLoadFromXML(CallBackTable,
                        parent, _T("IDR_XML_TASKLIST_L"));
  else
    wf = dlgLoadFromXML(CallBackTable,
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
