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
#include "LocalPath.hpp"
#include "Components.hpp"
#include "Task/ProtectedTaskManager.hpp"
#include "Task/TaskPoints/StartPoint.hpp"
#include "Task/TaskPoints/FinishPoint.hpp"
#include "Task/Visitors/TaskVisitor.hpp"
#include "Logger/Logger.hpp"
#include "Gauge/TaskView.hpp"

#include <assert.h>

static SingleWindow *parent_window;
static WndForm *wf = NULL;
static WndFrame* wTaskView = NULL;
static OrderedTask* ordered_task = NULL;
static bool task_modified = false;

static bool
CommitTaskChanges()
{
  if (!task_modified)
    return true;

  if (!ordered_task->task_size() || ordered_task->check_task()) {
    MessageBoxX(_("Active task modified"),
                _T("Task Manager"), MB_OK);

    ordered_task->check_duplicate_waypoints(way_points);
    protected_task_manager.task_commit(*ordered_task);
    protected_task_manager.task_save_default();

    task_modified = false;
    return true;
  } else if (MessageBoxX(_("Task not valid. Changes will be lost."),
                         _("Task Manager"),
                         MB_YESNO | MB_ICONQUESTION) == IDYES) {
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
  if (CommitTaskChanges())
    wf->SetModalResult(mrOK);
}

static void OnEditClicked(WindowControl * Sender)
{
  (void)Sender;
  task_modified |= dlgTaskEditShowModal(*parent_window, &ordered_task);
  if (task_modified)
    RefreshView();
}

static void OnListClicked(WindowControl * Sender)
{
  (void)Sender;
  task_modified |= dlgTaskListShowModal(*parent_window, &ordered_task);
  if (task_modified)
    RefreshView();
}

static void
OnDeclareClicked(WindowControl * Sender)
{
  (void)Sender;
  logger.LoggerDeviceDeclare(*ordered_task);
}


static void
OnTaskPaint(WindowControl *Sender, Canvas &canvas)
{
  PaintTask(canvas, Sender->get_client_rect(), *ordered_task,
            XCSoarInterface::Basic().Location,
            XCSoarInterface::SettingsMap(), terrain);
}

static CallBackTableEntry CallBackTable[] = {
  DeclareCallBackEntry(OnCloseClicked),
  DeclareCallBackEntry(OnEditClicked),
  DeclareCallBackEntry(OnListClicked),
  DeclareCallBackEntry(OnDeclareClicked),
  DeclareCallBackEntry(OnTaskPaint),
  DeclareCallBackEntry(NULL)
};

void
dlgTaskManagerShowModal(SingleWindow &parent)
{
  parent_window = &parent;

  if (Layout::landscape)
    wf = LoadDialog(CallBackTable,
                        parent,
                        _T("IDR_XML_TASKMANAGER_L"));
  else
    wf = LoadDialog(CallBackTable,
                        parent,
                        _T("IDR_XML_TASKMANAGER"));

  if (!wf)
    return;

  ordered_task = protected_task_manager.task_clone();
  task_modified = false;

  wTaskView = (WndFrame*)wf->FindByName(_T("frmTaskView"));
  if (!wTaskView)
    return;

  wf->ShowModal();

  delete wf;
  delete ordered_task;
}
