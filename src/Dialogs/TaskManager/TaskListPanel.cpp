/*
Copyright_License {

  XCSoar Glide Computer - http://www.xcsoar.org/
  Copyright (C) 2000-2012 The XCSoar Project
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

#include "TaskListPanel.hpp"
#include "Internal.hpp"
#include "Dialogs/CallBackTable.hpp"
#include "Dialogs/Message.hpp"
#include "Dialogs/TextEntry.hpp"
#include "Dialogs/dlgTaskHelpers.hpp"
#include "Form/Form.hpp"
#include "Form/Frame.hpp"
#include "Form/List.hpp"
#include "Form/Draw.hpp"
#include "Form/Tabbed.hpp"
#include "Form/TabBar.hpp"
#include "Task/TaskStore.hpp"
#include "Components.hpp"
#include "LocalPath.hpp"
#include "Gauge/TaskView.hpp"
#include "OS/FileUtil.hpp"
#include "Logger/ExternalLogger.hpp"
#include "UIGlobals.hpp"
#include "Look/MapLook.hpp"
#include "Simulator.hpp"
#include "Language/Language.hpp"
#include "Interface.hpp"
#include "Screen/Layout.hpp"
#include "Device/Declaration.hpp"
#include "Engine/Task/Tasks/OrderedTask.hpp"
#include "Engine/Waypoint/Waypoints.hpp"

#ifdef ENABLE_OPENGL
#include "Screen/OpenGL/Scissor.hpp"
#endif

#include <assert.h>
#include <windef.h>

/** XXX this hack is needed because the form callbacks don't get a
    context pointer - please refactor! */
static TaskListPanel *instance;

/**
 * used for browsing saved tasks
 * must be valid (2 task points)
 * @return NULL if no valid task at cursor, else pointer to task;
 */
OrderedTask *
TaskListPanel::get_cursor_task()
{
  const unsigned cursor_index = wTasks->GetCursorIndex();
  if (cursor_index >= task_store->Size())
    return NULL;

  OrderedTask *ordered_task = task_store->GetTask(cursor_index,
                                                  CommonInterface::GetComputerSettings().task);

  if (!ordered_task)
    return NULL;

  if (!ordered_task->CheckTask()) {
    delete ordered_task;
    ordered_task = NULL;
  }

  return ordered_task;
}

const TCHAR *
TaskListPanel::get_cursor_name()
{
  const unsigned cursor_index = wTasks->GetCursorIndex();
  if (cursor_index >= task_store->Size())
    return _T("");

  return task_store->GetName(cursor_index);
}

OrderedTask *
TaskListPanel::get_task_to_display()
{
  return (browse_tabbed->GetCurrentPage() == 0) ?
          *active_task :
          get_cursor_task();
}

void
TaskListPanel::OnTaskPaint(WndOwnerDrawFrame *Sender, Canvas &canvas)
{
  OrderedTask* ordered_task = get_task_to_display();

  if (ordered_task == NULL) {
    canvas.ClearWhite();
    return;
  }

#ifdef ENABLE_OPENGL
  /* enable clipping */
  GLCanvasScissor scissor(canvas);
#endif

  const MapLook &look = UIGlobals::GetMapLook();
  const NMEAInfo &basic = CommonInterface::Basic();
  PaintTask(canvas, Sender->GetClientRect(), *ordered_task,
            basic.location_available, basic.location,
            XCSoarInterface::GetMapSettings(),
            look.task, look.airspace,
            terrain, &airspace_database);
}

static void
OnTaskPaint(WndOwnerDrawFrame *Sender, Canvas &canvas)
{
  instance->OnTaskPaint(Sender, canvas);
}

void
TaskListPanel::OnTaskPaintListItem(Canvas &canvas, const PixelRect rc,
                                   unsigned DrawListIndex)
{
  assert(DrawListIndex <= task_store->Size());

  const TCHAR *name = task_store->GetName(DrawListIndex);

  canvas.text(rc.left + Layout::FastScale(2),
              rc.top + Layout::FastScale(2), name);
}

static void
OnTaskPaintListItem(Canvas &canvas, const PixelRect rc, unsigned DrawListIndex)
{
  instance->OnTaskPaintListItem(canvas, rc, DrawListIndex);
}

void
TaskListPanel::RefreshView()
{
  wTasks->SetLength(task_store->Size());
  wTaskView->Invalidate();

  WndFrame* wSummary = (WndFrame*)form.FindByName(_T("frmSummary1"));
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

void
TaskListPanel::SaveTask()
{
  (*active_task)->GetFactory().CheckAddFinish();

  if ((*active_task)->CheckTask()) {
    if (!OrderedTaskSave(*(SingleWindow *)wf.GetRootOwner(), **active_task))
      return;

    task_store->Scan();
    RefreshView();
  } else {
    ShowMessageBox(getTaskValidationErrors(
        (*active_task)->GetFactory().GetValidationErrors()), _("Task not saved"),
        MB_ICONEXCLAMATION);
  }
}

void
TaskListPanel::LoadTask()
{
  const OrderedTask* orig = get_cursor_task();
  if (orig == NULL)
    return;

  tstring text = _("Load the selected task?");
  text += _T("\n(");
  text += get_cursor_name();
  text += _T(")");

  if (ShowMessageBox(text.c_str(), _("Task Browser"),
                  MB_YESNO | MB_ICONQUESTION) != IDYES)
    return;

  // create new task first to guarantee pointers are different
  OrderedTask* temptask = orig->Clone(CommonInterface::GetComputerSettings().task);
  delete *active_task;
  *active_task = temptask;
  RefreshView();
  *task_modified = true;

  tab_bar.SetCurrentPage(dlgTaskManager::GetTurnpointTab());
  tab_bar.SetFocus();
}

void
TaskListPanel::DeleteTask()
{
  const unsigned cursor_index = wTasks->GetCursorIndex();
  if (cursor_index >= task_store->Size())
    return;

  const TCHAR *fname = task_store->GetName(cursor_index);
  tstring upperstring = fname;
  std::transform(upperstring.begin(), upperstring.end(), upperstring.begin(),
      ::toupper);

  if (upperstring.find(_T(".CUP")) != tstring::npos) {
    ShowMessageBox(_("Can't delete .CUP files"), _("Delete Error"),
        MB_ICONEXCLAMATION);
    return;
  }


  tstring text = _("Delete the selected task?");
  text += _T("\n(");
  text += fname;
  text += _T(")");

  if (ShowMessageBox(text.c_str(), _("Task Browser"),
                  MB_YESNO | MB_ICONQUESTION) != IDYES)
    return;

  File::Delete(task_store->GetPath(cursor_index));

  task_store->Scan();
  RefreshView();
}

static bool
ClearSuffix(TCHAR *p, const TCHAR *suffix)
{
  size_t length = _tcslen(p);
  size_t suffix_length = _tcslen(suffix);
  if (length <= suffix_length)
    return false;

  TCHAR *q = p + length - suffix_length;
  if (_tcsicmp(q, suffix) != 0)
    return false;

  *q = _T('\0');
  return true;
}

void
TaskListPanel::RenameTask()
{
  const unsigned cursor_index = wTasks->GetCursorIndex();
  if (cursor_index >= task_store->Size())
    return;

  const TCHAR *oldname = task_store->GetName(cursor_index);
  StaticString<40> newname(oldname);

  if (ClearSuffix(newname.buffer(), _T(".cup"))) {
    ShowMessageBox(_("Can't rename .CUP files"), _("Rename Error"),
        MB_ICONEXCLAMATION);
    return;
  }

  ClearSuffix(newname.buffer(), _T(".tsk"));

  if (!TextEntryDialog(*(SingleWindow *)wf.GetRootOwner(), newname))
    return;

  newname.append(_T(".tsk"));

  TCHAR newpath[MAX_PATH];
  LocalPath(newpath, _T("tasks"));
  Directory::Create(newpath);
  LocalPath(newpath, _T("tasks"), newname.c_str());

  File::Rename(task_store->GetPath(cursor_index), newpath);

  task_store->Scan();
  RefreshView();
}

void
TaskListPanel::OnManageClicked()
{
  dlgTaskManager::TaskViewRestore(wTaskView);
  browse_tabbed->SetCurrentPage(0);
  RefreshView();
}

class WndButton;

static void
OnManageClicked(gcc_unused WndButton &Sender)
{
  instance->OnManageClicked();
}

void
TaskListPanel::OnBrowseClicked()
{
  if (!lazy_loaded) {
    lazy_loaded = true;
    // Scan XCSoarData for available tasks
    task_store->Scan();
  }

  dlgTaskManager::TaskViewRestore(wTaskView);
  browse_tabbed->SetCurrentPage(1);
  RefreshView();
}

static void
OnBrowseClicked(gcc_unused WndButton &Sender)
{
  instance->OnBrowseClicked();
}

void
TaskListPanel::OnNewTaskClicked()
{
  if (((*active_task)->TaskSize() < 2) ||
      (ShowMessageBox(_("Create new task?"), _("Task New"),
                   MB_YESNO|MB_ICONQUESTION) == IDYES)) {
    (*active_task)->Clear();
    (*active_task)->SetFactory(XCSoarInterface::GetComputerSettings().task.task_type_default);
    *task_modified = true;
    tab_bar.SetCurrentPage(dlgTaskManager::GetPropertiesTab());
    tab_bar.SetFocus();
  }
}

static void
OnNewTaskClicked(gcc_unused WndButton &Sender)
{
  instance->OnNewTaskClicked();
}

static void
OnSaveClicked(gcc_unused WndButton &Sender)
{
  instance->SaveTask();
}

static void
OnLoadClicked(gcc_unused WndButton &Sender)
{
  instance->LoadTask();
}

static void
OnDeleteClicked(gcc_unused WndButton &Sender)
{
  instance->DeleteTask();
}

static void
OnRenameClicked(gcc_unused WndButton &Sender)
{
  instance->RenameTask();
}

static void
OnTaskListEnter(gcc_unused unsigned ItemIndex)
{
  instance->LoadTask();
}

static void
OnTaskCursorCallback(gcc_unused unsigned i)
{
  instance->RefreshView();
}

void
TaskListPanel::OnDeclareClicked()
{
  if (!(*active_task)->CheckTask()) {
    const AbstractTaskFactory::TaskValidationErrorVector errors =
      (*active_task)->GetFactory().GetValidationErrors();
    ShowMessageBox(getTaskValidationErrors(errors), _("Declare task"),
                MB_ICONEXCLAMATION);
    return;
  }

  const ComputerSettings &settings = CommonInterface::GetComputerSettings();
  Declaration decl(settings.logger, settings.plane, *active_task);
  ExternalLogger::Declare(decl, way_points.GetHome());
}

static void
OnDeclareClicked(gcc_unused WndButton &Sender)
{
  instance->OnDeclareClicked();
}

void
TaskListPanel::OnTaskViewClick()
{
  if (!fullscreen) {
    const UPixelScalar xoffset = (Layout::landscape ? tab_bar.GetTabWidth() : 0);
    const UPixelScalar yoffset = (!Layout::landscape ? tab_bar.GetTabHeight() : 0);
    wTaskView->Move(xoffset, yoffset,
                    wf.GetClientAreaWindow().GetWidth() - xoffset,
                    wf.GetClientAreaWindow().GetHeight() - yoffset);
    fullscreen = true;
    wTaskView->ShowOnTop();
  } else {
    wTaskView->Move(TaskViewRect.left, TaskViewRect.top,
                    TaskViewRect.right - TaskViewRect.left,
                    TaskViewRect.bottom - TaskViewRect.top);
    fullscreen = false;
  }
  wTaskView->Invalidate();
}

static bool
OnTaskViewClick(gcc_unused WndOwnerDrawFrame *Sender,
                gcc_unused PixelScalar x, gcc_unused PixelScalar y)
{
  instance->OnTaskViewClick();
  return true;
}

void
TaskListPanel::ReClick()
{
  if (browse_tabbed->GetCurrentPage() == 0) // manage page
    dlgTaskManager::OnTaskViewClick(wTaskView, 0, 0);
  else {
    browse_tabbed->SetCurrentPage(0);
    dlgTaskManager::TaskViewRestore(wTaskView);
  }
  RefreshView();
}

static gcc_constexpr_data CallBackTableEntry task_list_callbacks[] = {
  DeclareCallBackEntry(OnBrowseClicked),
  DeclareCallBackEntry(OnNewTaskClicked),
  DeclareCallBackEntry(OnSaveClicked),
  DeclareCallBackEntry(OnManageClicked),
  DeclareCallBackEntry(OnLoadClicked),
  DeclareCallBackEntry(OnDeleteClicked),
  DeclareCallBackEntry(OnDeclareClicked),
  DeclareCallBackEntry(OnRenameClicked),
  DeclareCallBackEntry(OnTaskPaint),

  DeclareCallBackEntry(NULL)
};

void
TaskListPanel::Prepare(ContainerWindow &parent, const PixelRect &rc)
{
  LoadWindow(task_list_callbacks, parent,
             Layout::landscape
             ? _T("IDR_XML_TASKLIST_L") : _T("IDR_XML_TASKLIST"));

  instance = this;

  task_store = new TaskStore();
  lazy_loaded = false;

  browse_tabbed = ((TabbedControl *)form.FindByName(_T("tabbedManage")));
  assert(browse_tabbed != NULL);

  if (is_simulator())
    /* cannot communicate with real devices in simulator mode */
    form.FindByName(_T("cmdDeclare"))->SetEnabled(false);

  // Save important control pointers
  wTaskView = (WndOwnerDrawFrame*)form.FindByName(_T("frmTaskView1"));
  assert(wTaskView != NULL);

  TaskViewRect = wTaskView->GetPosition();
  wTaskView->SetOnMouseDownNotify(::OnTaskViewClick);
  fullscreen = false;

  wTasks = (ListControl*)form.FindByName(_T("frmTasks"));
  assert(wTasks != NULL);

  // Set callbacks
  wTasks->SetActivateCallback(OnTaskListEnter);
  wTasks->SetPaintItemCallback(::OnTaskPaintListItem);
  wTasks->SetCursorCallback(OnTaskCursorCallback);
}

void
TaskListPanel::Unprepare()
{
  delete task_store;
  XMLWidget::Unprepare();
}

void
TaskListPanel::Show(const PixelRect &rc)
{
  browse_tabbed->SetCurrentPage(0);
  wTasks->SetCursorIndex(0); // so Save & Declare are always available
  dlgTaskManager::TaskViewRestore(wTaskView);
  RefreshView();
  XMLWidget::Show(rc);
}
