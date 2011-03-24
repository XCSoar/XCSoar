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
#include "Task/TaskStore.hpp"
#include "Task/ProtectedTaskManager.hpp"
#include "Components.hpp"
#include "LocalPath.hpp"
#include "Gauge/TaskView.hpp"
#include "OS/FileUtil.hpp"
#include "Logger/Logger.hpp"

#include <assert.h>

static WndForm *wf = NULL;
static TabBarControl* wTabBar;
static WndListFrame* wTasks = NULL;
static WndOwnerDrawFrame* wTaskView = NULL;
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

static bool
cursor_at_active_task()
{
  return (wTasks->GetCursorIndex() == 0);
}

static OrderedTask*
get_cursor_task()
{
  if (cursor_at_active_task())
    return *active_task;

  if (get_cursor_index() > task_store.size())
    return NULL;

  return task_store.get_task(get_cursor_index() - 1);
}

static const TCHAR *
get_cursor_name()
{
  if (cursor_at_active_task())
    return _T("(Active Task)");

  if (get_cursor_index() > task_store.size())
    return _T("");

  return task_store.get_name(get_cursor_index() - 1);
}

void
pnlTaskList::OnTaskPaint(WndOwnerDrawFrame *Sender, Canvas &canvas)
{
  PixelRect rc = Sender->get_client_rect();

  OrderedTask* ordered_task = get_cursor_task();
  if (ordered_task == NULL || !ordered_task->check_task()) {
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

  const TCHAR *name;
  if (DrawListIndex == 0)
    name = _T("(Active Task)");
  else
    name = task_store.get_name(DrawListIndex-1);

  canvas.text(rc.left + Layout::FastScale(2),
              rc.top + Layout::FastScale(2), name);
}

static void
RefreshView()
{
  wTasks->SetLength(task_store.size() + 1);
  wTaskView->invalidate();

  WndFrame* wSummary = (WndFrame*)wf->FindByName(_T("frmSummary1"));
  assert(wSummary != NULL);

  OrderedTask* ordered_task = get_cursor_task();
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
  if (!cursor_at_active_task())
    return;

  (*active_task)->get_factory().CheckAddFinish();

  if (((*active_task)->task_size() > 1) && (*active_task)->check_task()) {
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
  if (cursor_at_active_task())
    return;

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
LoadOrSaveTask()
{
  if (cursor_at_active_task())
    SaveTask();
  else
    LoadTask();
}

static void
DeleteTask()
{
  if (cursor_at_active_task())
    return;

  const TCHAR *fname = get_cursor_name();
  tstring upperstring = fname;
  std::transform(upperstring.begin(), upperstring.end(), upperstring.begin(),
      ::toupper);

  if (upperstring.find(_T(".CUP")) != tstring::npos) {
    MessageBoxX(_T("Can't delete .CUP files"), _("Delete Error"),
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
  if (cursor_at_active_task())
    return;

  const TCHAR *oldname = get_cursor_name();
  tstring newname = oldname;
  tstring upperstring = newname;
  std::transform(upperstring.begin(), upperstring.end(), upperstring.begin(),
      ::toupper);

  if (upperstring.find(_T(".CUP")) != tstring::npos) {
    MessageBoxX(_T("Can't rename .CUP files"), _("Rename Error"),
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

static void
UpdateButtons()
{
  bool at_active_task = cursor_at_active_task();

  WndButton* wbLoadSave = (WndButton*)wf->FindByName(_T("cmdLoadSave"));
  assert(wbLoadSave != NULL);
  wbLoadSave->SetCaption(at_active_task ?_("Save") : _("Load"));

  WndButton* wbDelete = (WndButton*)wf->FindByName(_T("cmdDelete"));
  assert(wbDelete != NULL);
  wbDelete->set_enabled(!at_active_task);

  WndButton* wbRename = (WndButton*)wf->FindByName(_T("cmdRename"));
  assert(wbRename != NULL);
  wbRename->set_enabled(!at_active_task);

  WndButton* wbDeclare = (WndButton*)wf->FindByName(_T("cmdDeclare"));
  assert(wbDeclare != NULL);
  wbDeclare->set_enabled(at_active_task);
}

void
pnlTaskList::OnLoadSaveClicked(WndButton &Sender)
{
  LoadOrSaveTask();
}

void
pnlTaskList::OnDeleteClicked(WndButton &Sender)
{
  DeleteTask();
}

void
pnlTaskList::OnRenameClicked(WndButton &Sender)
{
  RenameTask();
}

void
pnlTaskList::OnTaskListEnter(unsigned ItemIndex)
{
  LoadOrSaveTask();
}

void
pnlTaskList::OnTaskCursorCallback(unsigned i)
{
  UpdateButtons();
  RefreshView();
}

bool
pnlTaskList::OnDeclareClicked(WndButton &Sender)
{
  logger.LoggerDeviceDeclare(**active_task);
  return false;
}

bool
pnlTaskList::OnTaskViewClick(WndOwnerDrawFrame *Sender, int x, int y)
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
pnlTaskList::OnTabPreShow(TabBarControl::EventType EventType)
{
  if (!lazy_loaded) {
    lazy_loaded = true;
    // Scan XCSoarData for available tasks
    task_store.scan();
  }
  wTasks->SetCursorIndex(0); // so Save & Declare are always available
  UpdateButtons();
  dlgTaskManager::TaskViewRestore(wTaskView);
  RefreshView();
  return true;
}

void
pnlTaskList::OnTabReClick()
{
  dlgTaskManager::OnTaskViewClick(wTaskView, 0, 0);
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
