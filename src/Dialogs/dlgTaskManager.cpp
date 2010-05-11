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
#include "MainWindow.hpp"
#include "LocalPath.hpp"
#include "DataField/FileReader.hpp"
#include "Components.hpp"
#include "StringUtil.hpp"
#include "TaskClientUI.hpp"
#include "Task/TaskPoints/StartPoint.hpp"
#include "Task/TaskPoints/FinishPoint.hpp"
#include "Task/Visitors/TaskVisitor.hpp"
#include "RenderTask.hpp"
#include "RenderTaskPoint.hpp"
#include "RenderObservationZone.hpp"
#include "TaskClientUI.hpp"
#include "Screen/Chart.hpp"
#include "ChartProjection.hpp"

#include <assert.h>

static SingleWindow *parent_window;
static WndForm *wf=NULL;
static WndFrame* wTaskView = NULL;
static OrderedTask* ordered_task= NULL;
static bool task_modified = false;

static bool
CommitTaskChanges()
{
  if (!task_modified)
    return true;

  if (!ordered_task->task_size() || ordered_task->check_task()) {

    MessageBoxX (gettext(TEXT("Active task modified")),
                 TEXT("Task Manager"), MB_OK);

    task_ui.task_commit(*ordered_task);
    task_ui.task_save_default();

    task_modified = false;
    return true;

  } else if (MessageBoxX(gettext(_T("Task not valid. Changes will be lost.")),
                    gettext(_T("Task Manager")),
                    MB_YESNO|MB_ICONQUESTION) == IDYES) {
    return true;
  }
  return false;
}

static void
RefreshView()
{
  wTaskView->invalidate();
}

static void OnCloseClicked(WindowControl * Sender)
{
  (void)Sender;

  if (CommitTaskChanges()) {
    wf->SetModalResult(mrOK);
  }
}

static void OnEditClicked(WindowControl * Sender)
{
  (void)Sender;
  task_modified |= dlgTaskEditShowModal(*parent_window, &ordered_task);
  if (task_modified) {
    RefreshView();
  }
}

static void OnListClicked(WindowControl * Sender)
{
  (void)Sender;
  task_modified |= dlgTaskListShowModal(*parent_window, &ordered_task);
  if (task_modified) {
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

static CallBackTableEntry_t CallBackTable[]={
  DeclareCallBackEntry(OnCloseClicked),
  DeclareCallBackEntry(OnEditClicked),
  DeclareCallBackEntry(OnListClicked),
  DeclareCallBackEntry(OnTaskPaint),
  DeclareCallBackEntry(NULL)
};

void
dlgTaskManagerShowModal(SingleWindow &parent)
{
  parent_window = &parent;

  wf = NULL;

  if (Layout::landscape) {
    wf = dlgLoadFromXML(CallBackTable,
                        _T("dlgTaskManager_L.xml"),
                        parent,
                        _T("IDR_XML_TASKMANAGER_L"));
  } else {
    wf = dlgLoadFromXML(CallBackTable,
                        _T("dlgTaskManager.xml"),
                        parent,
                        _T("IDR_XML_TASKMANAGER"));
  }

  if (!wf) return;
  assert(wf!=NULL);

  ordered_task = task_ui.task_clone();
  task_modified = false;

  wTaskView = (WndFrame*)wf->FindByName(_T("frmTaskView"));

  wf->ShowModal();

  delete wf;
  delete ordered_task;
  wf = NULL;
}
