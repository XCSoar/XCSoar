/*
Copyright_License {

  XCSoar Glide Computer - http://www.xcsoar.org/
  Copyright (C) 2000-2010 The XCSoar Project
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

#include "Dialogs/dlgTaskManager.hpp"
#include "Dialogs/Task.hpp"
#include "Dialogs/Internal.hpp"
#include "Dialogs/dlgTaskHelpers.hpp"
#include "Screen/Layout.hpp"
#include "Screen/Key.h"
#include "Components.hpp"
#include "Gauge/TaskView.hpp"
#include "Task/ProtectedTaskManager.hpp"
#include "Screen/Fonts.hpp"
#include "Screen/Graphics.hpp"
#include "Task/TaskStore.hpp"
#include "LocalPath.hpp"
#include "OS/FileUtil.hpp"
#include "Logger/Logger.hpp"
#include "Protection.hpp"
#include "Look/Look.hpp"
#include "MainWindow.hpp"
#include "Form/TabBar.hpp"
#include "Form/Panel.hpp"

#ifdef ENABLE_OPENGL
#include "Screen/OpenGL/Scissor.hpp"
#endif

#include <assert.h>
#include <stdio.h>

static WndForm *wf = NULL;

static TabBarControl* wTabBar = NULL;
static OrderedTask* active_task = NULL;
static bool task_modified = false;
static bool fullscreen;
static PixelRect TaskViewRect;
static unsigned TurnpointTab = 0;
static unsigned PropertiesTab = 0;

unsigned
dlgTaskManager::GetTurnpointTab()
{
  return TurnpointTab;
}

unsigned
dlgTaskManager::GetPropertiesTab()
{
  return PropertiesTab;
}

void
dlgTaskManager::SetTitle()
{
  TCHAR title[99];

  _stprintf(title, _T("%s - %s"), _("Task Manager"),
            wTabBar->GetButtonCaption((wTabBar->GetCurrentPage())));
  wf->SetCaption(title);
}

bool
dlgTaskManager::OnTaskViewClick(WndOwnerDrawFrame *Sender,
                                PixelScalar x, PixelScalar y)
{
  if (!fullscreen) {
    const UPixelScalar xoffset = Layout::landscape ? wTabBar->GetTabWidth() : 0;
    const UPixelScalar yoffset = !Layout::landscape ? wTabBar->GetTabHeight() : 0;
    Sender->move(xoffset, yoffset, wf->GetClientAreaWindow().get_width() - xoffset,
                    wf->GetClientAreaWindow().get_height() - yoffset);
    fullscreen = true;
    Sender->show_on_top();
  } else {
    Sender->move(TaskViewRect.left, TaskViewRect.top,
                    TaskViewRect.right - TaskViewRect.left,
                    TaskViewRect.bottom - TaskViewRect.top);
    fullscreen = false;
  }
  Sender->invalidate();
  return true;
}

void
dlgTaskManager::TaskViewRestore(WndOwnerDrawFrame *wTaskView)
{
  fullscreen = false;
  wTaskView->move(TaskViewRect.left, TaskViewRect.top,
      TaskViewRect.right - TaskViewRect.left,
      TaskViewRect.bottom - TaskViewRect.top);
}

void
dlgTaskManager::OnTaskPaint(WndOwnerDrawFrame *Sender, Canvas &canvas)
{
#ifdef ENABLE_OPENGL
  /* enable clipping */
  GLCanvasScissor scissor(canvas);
#endif

  const Look &look = *CommonInterface::main_window.look;
  PaintTask(canvas, Sender->get_client_rect(), *active_task,
            XCSoarInterface::Basic().location,
            XCSoarInterface::SettingsMap(),
            look.task, look.airspace,
            terrain);
}

void
dlgTaskManager::OnBlackBarPaint(WndOwnerDrawFrame *Sender, Canvas &canvas)
{
  canvas.clear(COLOR_BLACK);
  if (wTabBar->has_focus()) {
    PixelRect rcFocus;
    rcFocus.top = rcFocus.left = 0;
    rcFocus.right = canvas.get_width();
    rcFocus.bottom = canvas.get_height();
    canvas.draw_focus(rcFocus);
  }
}

bool
dlgTaskManager::CommitTaskChanges()
{
  if (!task_modified)
    return true;

  task_modified |= active_task->GetFactory().CheckAddFinish();

  if (!active_task->TaskSize() || active_task->CheckTask()) {

    { // this must be done in thread lock because it potentially changes the
      // waypoints database
      ScopeSuspendAllThreads suspend;
      active_task->CheckDuplicateWaypoints(way_points);
      way_points.optimise();
    }

    protected_task_manager->TaskCommit(*active_task);
    protected_task_manager->TaskSaveDefault();

    task_modified = false;
    return true;
  }

  MessageBoxX(getTaskValidationErrors(
    active_task->GetFactory().getValidationErrors()),
    _("Validation Errors"), MB_ICONEXCLAMATION);

  return (MessageBoxX(_("Task not valid. Changes will be lost.\nContinue?"),
                      _("Task Manager"), MB_YESNO | MB_ICONQUESTION) == IDYES);
}

bool
dlgTaskManager::OnClose()
{
  if (CommitTaskChanges()) {
    wf->SetModalResult(mrOK);
    return true;
  }

  return false;
}

void
dlgTaskManagerShowModal(SingleWindow &parent)
{
  dlgTaskManager::dlgTaskManagerShowModal(parent);
}

const CallBackTableEntry dlgTaskManager::CallBackTable[] = {
  DeclareCallBackEntry(dlgTaskManager::OnTaskPaint),
  DeclareCallBackEntry(dlgTaskManager::OnBlackBarPaint),

  DeclareCallBackEntry(pnlTaskEdit::OnMakeFinish),
  DeclareCallBackEntry(pnlTaskEdit::OnMoveUpClicked),
  DeclareCallBackEntry(pnlTaskEdit::OnMoveDownClicked),
  DeclareCallBackEntry(pnlTaskEdit::OnEditTurnpointClicked),
  DeclareCallBackEntry(pnlTaskEdit::OnClearAllClicked),
  DeclareCallBackEntry(pnlTaskEdit::OnTabPreShow),

  DeclareCallBackEntry(pnlTaskProperties::OnTaskTypeData),

  DeclareCallBackEntry(pnlTaskList::OnBrowseClicked),
  DeclareCallBackEntry(pnlTaskList::OnNewTaskClicked),
  DeclareCallBackEntry(pnlTaskList::OnSaveClicked),
  DeclareCallBackEntry(pnlTaskList::OnManageClicked),
  DeclareCallBackEntry(pnlTaskList::OnLoadClicked),
  DeclareCallBackEntry(pnlTaskList::OnDeleteClicked),
  DeclareCallBackEntry(pnlTaskList::OnDeclareClicked),
  DeclareCallBackEntry(pnlTaskList::OnRenameClicked),
  DeclareCallBackEntry(pnlTaskList::OnTaskPaint),
  DeclareCallBackEntry(pnlTaskList::OnTabPreShow),

  DeclareCallBackEntry(pnlTaskProperties::OnFAIFinishHeightData),

  DeclareCallBackEntry(pnlTaskManagerClose::OnCloseClicked),
  DeclareCallBackEntry(pnlTaskManagerClose::OnRevertClicked),

  DeclareCallBackEntry(pnlTaskCalculator::OnMacCreadyData),
  DeclareCallBackEntry(pnlTaskCalculator::OnTargetClicked),
  DeclareCallBackEntry(pnlTaskCalculator::OnCruiseEfficiencyData),
  DeclareCallBackEntry(pnlTaskCalculator::OnWarningPaint),


  DeclareCallBackEntry(NULL)
};

void
dlgTaskManager::RevertTask()
{
  // create new task first to guarantee pointers are different
  OrderedTask* temptask = protected_task_manager->TaskClone();
  delete active_task;
  active_task = temptask;
  task_modified = false;
}

void
dlgTaskManager::dlgTaskManagerShowModal(SingleWindow &parent)
{
  if (protected_task_manager == NULL)
    return;

  wf = LoadDialog(CallBackTable, parent,
                  Layout::landscape ?
                  _T("IDR_XML_TASKMANAGER_L") : _T("IDR_XML_TASKMANAGER"));

  assert(wf != NULL);

  active_task = protected_task_manager->TaskClone();
  task_modified = false;

  // Load tabs
  wTabBar = (TabBarControl*)wf->FindByName(_T("TabBar"));
  assert(wTabBar != NULL);

  wTabBar->SetClientOverlapTabs(true);

  if (!Layout::landscape) {
    WndOwnerDrawFrame* wBlackRect =
        (WndOwnerDrawFrame*)wf->FindByName(_T("frmBlackRect"));
    assert(wBlackRect);
    const UPixelScalar TabLineHeight = wTabBar->GetTabLineHeight();
    wBlackRect->move(0,
                     wTabBar->GetTabHeight() - TabLineHeight - Layout::Scale(1),
                     wf->get_width() - wTabBar->GetTabWidth() + Layout::Scale(3),
                     TabLineHeight + Layout::Scale(2));
    wBlackRect->show_on_top();
  }

  Window* wProps =
    pnlTaskProperties::Load(parent, wTabBar, wf, &active_task, &task_modified);
  assert(wProps);

  Window* wClose =
    pnlTaskManagerClose::Load(parent, wTabBar, wf, &active_task, &task_modified);
  assert(wClose);

  Window* wCalculator =
    pnlTaskCalculator::Load(parent, wTabBar, wf, &task_modified);
  assert(wCalculator);

  Window* wEdit =
    pnlTaskEdit::Load(parent, wTabBar, wf, &active_task, &task_modified);
  assert(wEdit);

  WndOwnerDrawFrame* wTaskView =
      (WndOwnerDrawFrame*)wf->FindByName(_T("frmTaskView"));
  assert(wTaskView);
  TaskViewRect = wTaskView->get_position();

  Window* wLst =
    pnlTaskList::Load(parent, wTabBar, wf, &active_task, &task_modified);
  assert(wLst);

  const DialogTabStyle_t IconsStyle =
    CommonInterface::GetUISettings().dialog.tab_style;
  const Bitmap *CalcIcon = ((IconsStyle == dtIcon) ?
                             &Graphics::hBmpTabCalculator : NULL);
  const Bitmap *TurnPointIcon = ((IconsStyle == dtIcon) ?
                                  &Graphics::hBmpTabTask : NULL);
  const Bitmap *BrowseIcon = ((IconsStyle == dtIcon) ?
                               &Graphics::hBmpTabWrench : NULL);
  const Bitmap *PropertiesIcon = ((IconsStyle == dtIcon) ?
                                   &Graphics::hBmpTabSettings : NULL);

  if (Layout::landscape) {
    wTabBar->AddClient(wCalculator, _("Calculator"), false, CalcIcon, NULL,
                       pnlTaskCalculator::OnTabPreShow, dlgTaskManager::SetTitle);

    wTabBar->AddClient(wEdit, _("Turn Points"), false, TurnPointIcon, NULL,
                       pnlTaskEdit::OnTabPreShow, dlgTaskManager::SetTitle,
                       pnlTaskEdit::OnTabReClick);
    TurnpointTab = 1;

    wTabBar->AddClient(wLst, _("Manage"), false, BrowseIcon, NULL,
                       pnlTaskList::OnTabPreShow, dlgTaskManager::SetTitle,
                       pnlTaskList::OnTabReClick);

    wTabBar->AddClient(wProps, _("Rules"), false, PropertiesIcon,
                       pnlTaskProperties::OnTabPreHide,
                       pnlTaskProperties::OnTabPreShow, dlgTaskManager::SetTitle,
                       pnlTaskProperties::OnTabReClick);
    PropertiesTab = 3;

    wTabBar->AddClient(wClose, _("Close"), false, NULL, NULL,
                       pnlTaskManagerClose::OnTabPreShow, dlgTaskManager::SetTitle,
                       pnlTaskManagerClose::OnTabReClick);

    wTabBar->SetCurrentPage(0);
  } else {
    wTabBar->AddClient(wCalculator, _("Calculator"), false, CalcIcon, NULL,
                       pnlTaskCalculator::OnTabPreShow, dlgTaskManager::SetTitle);

    wTabBar->AddClient(wClose, _("Close"), false, NULL, NULL,
                       pnlTaskManagerClose::OnTabPreShow, dlgTaskManager::SetTitle,
                       pnlTaskManagerClose::OnTabReClick);

    wTabBar->AddClient(wEdit, _("Turn Points"), false, TurnPointIcon, NULL,
                       pnlTaskEdit::OnTabPreShow, dlgTaskManager::SetTitle,
                       pnlTaskEdit::OnTabReClick);
    TurnpointTab = 2;

    wTabBar->AddClient(wLst, _("Manage"), false, BrowseIcon, NULL,
                       pnlTaskList::OnTabPreShow, dlgTaskManager::SetTitle,
                       pnlTaskList::OnTabReClick);

    wTabBar->AddClient(wProps, _("Rules"), false, PropertiesIcon,
                       pnlTaskProperties::OnTabPreHide,
                       pnlTaskProperties::OnTabPreShow, dlgTaskManager::SetTitle,
                       pnlTaskProperties::OnTabReClick);
    PropertiesTab = 4;

    wTabBar->SetCurrentPage(0);
  }


  fullscreen = false;

  wf->ShowModal();

  pnlTaskList::DestroyTab();

  delete wf;
  delete active_task;
}
