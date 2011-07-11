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
#include "Dialogs/TextEntry.hpp"
#include "Screen/Layout.hpp"
#include "Dialogs/dlgTaskHelpers.hpp"
#include "Dialogs/dlgTaskManager.hpp"
#include "Form/Tabbed.hpp"
#include "Task/TaskStore.hpp"
#include "Task/ProtectedTaskManager.hpp"
#include "Components.hpp"
#include "LocalPath.hpp"
#include "Gauge/TaskView.hpp"
#include "OS/FileUtil.hpp"
#include "Logger/Logger.hpp"
#include "Simulator.hpp"

#include <assert.h>

static WndForm *wf = NULL;
static TabBarControl* wTabBar;
static WndListFrame* wTasks = NULL;
static WndOwnerDrawFrame* wTaskView = NULL;
static TabbedControl *browse_tabbed = NULL;
static TaskStore task_store;
static PixelRect TaskViewRect;
static bool fullscreen;

static OrderedTask** active_task = NULL;
static bool* task_modified = NULL;
bool lazy_loaded = false; // if store has been loaded first time tab displayed

static unsigned
get_cursor_index()
{
  return wTasks->GetCursorIndex();
}

/**
 * used for browsing saved tasks
 * must be valid (2 task points)
 * @return NULL if no valid task at cursor, else pointer to task;
 */
static OrderedTask*
get_cursor_task()
{
  if (get_cursor_index() >= task_store.size())
    return NULL;

  OrderedTask * ordered_task = task_store.get_task(get_cursor_index());

  if (!ordered_task)
    return NULL;

  if (!ordered_task->check_task()) {
    delete ordered_task;
    ordered_task = NULL;
  }

  return ordered_task;
}

static const TCHAR *
get_cursor_name()
{
  if (get_cursor_index() >= task_store.size())
    return _T("");

  return task_store.get_name(get_cursor_index());
}

static OrderedTask*
get_task_to_display()
{
  return (browse_tabbed->GetCurrentPage() == 0) ?
          *active_task :
          get_cursor_task();
}

void
pnlTaskList::OnTaskPaint(WndOwnerDrawFrame *Sender, Canvas &canvas)
{
  PixelRect rc = Sender->get_client_rect();

  OrderedTask* ordered_task = get_task_to_display();

  if (ordered_task == NULL) {
    canvas.clear_white();
    return;
  }
  PaintTask(canvas, rc, *ordered_task, XCSoarInterface::Basic().Location,
            XCSoarInterface::SettingsMap(), terrain);
}

void
pnlTaskList::OnTaskPaintListItem(Canvas &canvas, const PixelRect rc,
                                 unsigned DrawListIndex)
{
  assert(DrawListIndex <= task_store.size());

  const TCHAR *name = task_store.get_name(DrawListIndex);

  canvas.text(rc.left + Layout::FastScale(2),
              rc.top + Layout::FastScale(2), name);
}

static void
RefreshView()
{
  wTasks->SetLength(task_store.size());
  wTaskView->invalidate();

  WndFrame* wSummary = (WndFrame*)wf->FindByName(_T("frmSummary1"));
  assert(wSummary != NULL);

  OrderedTask* ordered_task = get_task_to_display();

  if (ordered_task == NULL) {
    wSummary->SetCaption(_T(""));
    return;
  }

  TCHAR text[300];
  OrderedTaskSummary(ordered_task, text, false);
  wSummary->SetCaption(text);
}

static void
SaveTask()
{
  (*active_task)->get_factory().CheckAddFinish();

  if ((*active_task)->check_task()) {
    if (!OrderedTaskSave(**active_task))
      return;

    task_store.scan();
    RefreshView();
  } else {
    MessageBoxX(getTaskValidationErrors(
        (*active_task)->get_factory().getValidationErrors()), _("Task not saved"),
        MB_ICONEXCLAMATION);
  }
}

static void
LoadTask()
{
  const OrderedTask* orig = get_cursor_task();
  if (orig == NULL)
    return;

  tstring text = _("Load the selected task?");
  text += _T("\n(");
  text += get_cursor_name();
  text += _T(")");

  if (MessageBoxX(text.c_str(), _("Task Browser"),
                  MB_YESNO | MB_ICONQUESTION) != IDYES)
    return;

  // create new task first to guarantee pointers are different
  OrderedTask* temptask = protected_task_manager->task_copy(*orig);
  delete *active_task;
  *active_task = temptask;
  RefreshView();
  *task_modified = true;

  wTabBar->SetCurrentPage(dlgTaskManager::GetTurnpointTab());
  wTabBar->set_focus();
}

static void
DeleteTask()
{
  const TCHAR *fname = get_cursor_name();
  tstring upperstring = fname;
  std::transform(upperstring.begin(), upperstring.end(), upperstring.begin(),
      ::toupper);

  if (upperstring.find(_T(".CUP")) != tstring::npos) {
    MessageBoxX(_("Can't delete .CUP files"), _("Delete Error"),
        MB_ICONEXCLAMATION);
    return;
  }


  tstring text = _("Delete the selected task?");
  text += _T("\n(");
  text += fname;
  text += _T(")");

  if (MessageBoxX(text.c_str(), _("Task Browser"),
                  MB_YESNO | MB_ICONQUESTION) != IDYES)
    return;

  TCHAR path[MAX_PATH];
  LocalPath(path, fname);
  File::Delete(path);

  task_store.scan();
  RefreshView();
}

static void
RenameTask()
{
  const TCHAR *oldname = get_cursor_name();
  tstring newname = oldname;
  tstring upperstring = newname;
  std::transform(upperstring.begin(), upperstring.end(), upperstring.begin(),
      ::toupper);

  if (upperstring.find(_T(".CUP")) != tstring::npos) {
    MessageBoxX(_("Can't rename .CUP files"), _("Rename Error"),
        MB_ICONEXCLAMATION);
    return;
  }

  if (upperstring.find(_T(".TSK")) != tstring::npos)
    newname = newname.substr(0, upperstring.find(_T(".TSK")));

  if (!dlgTextEntryShowModal(newname, 40))
    return;

  newname += _T(".tsk");

  TCHAR oldpath[MAX_PATH];
  TCHAR newpath[MAX_PATH];
  LocalPath(oldpath, oldname);
  LocalPath(newpath, newname.c_str());

  File::Rename(oldpath, newpath);

  task_store.scan();
  RefreshView();
}

void
pnlTaskList::OnManageClicked(gcc_unused WndButton &Sender)
{
  dlgTaskManager::TaskViewRestore(wTaskView);
  browse_tabbed->SetCurrentPage(0);
  RefreshView();
}

void
pnlTaskList::OnBrowseClicked(gcc_unused WndButton &Sender)
{
  if (!lazy_loaded) {
    lazy_loaded = true;
    // Scan XCSoarData for available tasks
    task_store.scan();
  }

  dlgTaskManager::TaskViewRestore(wTaskView);
  browse_tabbed->SetCurrentPage(1);
  RefreshView();
}

void
pnlTaskList::OnNewTaskClicked(gcc_unused WndButton &Sender)
{
  if (((*active_task)->task_size() < 2) ||
      (MessageBoxX(_("Create new task?"), _("Task New"),
                   MB_YESNO|MB_ICONQUESTION) == IDYES)) {
    (*active_task)->clear();
    (*active_task)->set_factory(XCSoarInterface::SettingsComputer().task_type_default);
    *task_modified = true;
    wTabBar->SetCurrentPage(dlgTaskManager::GetPropertiesTab());
    wTabBar->set_focus();
  }
}

void
pnlTaskList::OnSaveClicked(gcc_unused WndButton &Sender)
{
  SaveTask();
}

void
pnlTaskList::OnLoadClicked(gcc_unused WndButton &Sender)
{
  LoadTask();
}

void
pnlTaskList::OnDeleteClicked(gcc_unused WndButton &Sender)
{
  DeleteTask();
}

void
pnlTaskList::OnRenameClicked(gcc_unused WndButton &Sender)
{
  RenameTask();
}

void
pnlTaskList::OnTaskListEnter(gcc_unused unsigned ItemIndex)
{
  LoadTask();
}

void
pnlTaskList::OnTaskCursorCallback(gcc_unused unsigned i)
{
  RefreshView();
}

bool
pnlTaskList::OnDeclareClicked(gcc_unused WndButton &Sender)
{
  logger.LoggerDeviceDeclare(**active_task);
  return false;
}

bool
pnlTaskList::OnTaskViewClick(gcc_unused WndOwnerDrawFrame *Sender,
                             gcc_unused int x, gcc_unused int y)
{
  if (!fullscreen) {
    const unsigned xoffset = (Layout::landscape ? wTabBar->GetTabWidth() : 0);
    const unsigned yoffset = (!Layout::landscape ? wTabBar->GetTabHeight() : 0);
    wTaskView->move(xoffset, yoffset, wf->GetClientAreaWindow().get_width() - xoffset,
                    wf->GetClientAreaWindow().get_height() - yoffset);
    fullscreen = true;
    wTaskView->show_on_top();
  } else {
    wTaskView->move(TaskViewRect.left, TaskViewRect.top,
                    TaskViewRect.right - TaskViewRect.left,
                    TaskViewRect.bottom - TaskViewRect.top);
    fullscreen = false;
  }
  wTaskView->invalidate();
  return true;
}

bool
pnlTaskList::OnTabPreShow(gcc_unused TabBarControl::EventType EventType)
{
  browse_tabbed->SetCurrentPage(0);
  wTasks->SetCursorIndex(0); // so Save & Declare are always available
  dlgTaskManager::TaskViewRestore(wTaskView);
  RefreshView();
  return true;
}

void
pnlTaskList::OnTabReClick()
{
  if (browse_tabbed->GetCurrentPage() == 0) // manage page
    dlgTaskManager::OnTaskViewClick(wTaskView, 0, 0);
  else {
    browse_tabbed->SetCurrentPage(0);
    dlgTaskManager::TaskViewRestore(wTaskView);
  }
  RefreshView();
}

void
pnlTaskList::DestroyTab()
{
  task_store.clear();
}

Window*
pnlTaskList::Load(SingleWindow &parent, TabBarControl* _wTabBar, WndForm* _wf,
                  OrderedTask** task, bool* _task_modified)
{

  assert(_wTabBar);
  wTabBar = _wTabBar;

  assert(_wf);
  wf = _wf;

  assert(task);
  active_task = task;

  assert(_task_modified);
  task_modified = _task_modified;

  lazy_loaded = false;

  // Load the dialog
  Window *wList = LoadWindow(dlgTaskManager::CallBackTable, wf, *wTabBar,
                             Layout::landscape ?
                             _T("IDR_XML_TASKLIST_L") : _T("IDR_XML_TASKLIST"));
  assert(wList);

  browse_tabbed = ((TabbedControl *)wf->FindByName(_T("tabbedManage")));
  assert(browse_tabbed != NULL);

  if (is_simulator())
    /* cannot communicate with real devices in simulator mode */
    wf->FindByName(_T("cmdDeclare"))->set_enabled(false);

  // Save important control pointers
  wTaskView = (WndOwnerDrawFrame*)wf->FindByName(_T("frmTaskView1"));
  assert(wTaskView != NULL);

  TaskViewRect = wTaskView->get_position();
  wTaskView->SetOnMouseDownNotify(OnTaskViewClick);
  fullscreen = false;

  wTasks = (WndListFrame*)wf->FindByName(_T("frmTasks"));
  assert(wTasks != NULL);

  // Set callbacks
  wTasks->SetActivateCallback(pnlTaskList::OnTaskListEnter);
  wTasks->SetPaintItemCallback(pnlTaskList::OnTaskPaintListItem);
  wTasks->SetCursorCallback(pnlTaskList::OnTaskCursorCallback);

  return wList;
}
