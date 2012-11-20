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
#include "TaskMiscPanel.hpp"
#include "Internal.hpp"
#include "Dialogs/CallBackTable.hpp"
#include "Dialogs/Message.hpp"
#include "Dialogs/TextEntry.hpp"
#include "Dialogs/dlgTaskHelpers.hpp"
#include "Form/Form.hpp"
#include "Form/Button.hpp"
#include "Form/Frame.hpp"
#include "Form/List.hpp"
#include "Form/Draw.hpp"
#include "Form/TabBar.hpp"
#include "Task/TaskStore.hpp"
#include "Components.hpp"
#include "LocalPath.hpp"
#include "Gauge/TaskView.hpp"
#include "OS/FileUtil.hpp"
#include "UIGlobals.hpp"
#include "Look/MapLook.hpp"
#include "Language/Language.hpp"
#include "Interface.hpp"
#include "Screen/Canvas.hpp"
#include "Screen/Layout.hpp"
#include "Engine/Task/Ordered/OrderedTask.hpp"

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

void
TaskListPanel::OnTaskPaint(WndOwnerDrawFrame *Sender, Canvas &canvas)
{
  OrderedTask *ordered_task = get_cursor_task();

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
TaskListPanel::OnPaintItem(Canvas &canvas, const PixelRect rc,
                           unsigned DrawListIndex)
{
  assert(DrawListIndex <= task_store->Size());

  const TCHAR *name = task_store->GetName(DrawListIndex);

  canvas.DrawText(rc.left + Layout::FastScale(2),
                  rc.top + Layout::FastScale(2), name);
}

void
TaskListPanel::RefreshView()
{
  wTasks->SetLength(task_store->Size());

  if (wTaskView != NULL)
    wTaskView->Invalidate();

  WndFrame* wSummary = (WndFrame*)form.FindByName(_T("frmSummary1"));
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

void
TaskListPanel::DirtyList()
{
  if (task_store != NULL) {
    task_store->Scan(more);
    RefreshView();
  }
}

void
TaskListPanel::SaveTask()
{
  (*active_task)->GetFactory().CheckAddFinish();

  if ((*active_task)->CheckTask()) {
    if (!OrderedTaskSave(**active_task))
      return;

    task_store->Scan(more);
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

  task_store->Scan(more);
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
  if (!StringIsEqualIgnoreCase(q, suffix))
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

  if (!TextEntryDialog(newname))
    return;

  newname.append(_T(".tsk"));

  TCHAR newpath[MAX_PATH];
  LocalPath(newpath, _T("tasks"));
  Directory::Create(newpath);
  LocalPath(newpath, _T("tasks"), newname.c_str());

  File::Rename(task_store->GetPath(cursor_index), newpath);

  task_store->Scan(more);
  RefreshView();
}

void
TaskListPanel::OnMoreClicked()
{
  more = !more;

  more_button->SetCaption(more ? _("Less") : _("More"));

  task_store->Scan(more);
  RefreshView();
}

class WndButton;

static void
OnMoreClicked()
{
  instance->OnMoreClicked();
}

static void
OnLoadClicked()
{
  instance->LoadTask();
}

static void
OnDeleteClicked()
{
  instance->DeleteTask();
}

static void
OnRenameClicked()
{
  instance->RenameTask();
}

static constexpr CallBackTableEntry task_list_callbacks[] = {
  DeclareCallBackEntry(OnMoreClicked),
  DeclareCallBackEntry(OnLoadClicked),
  DeclareCallBackEntry(OnDeleteClicked),
  DeclareCallBackEntry(OnRenameClicked),

  DeclareCallBackEntry(NULL)
};

void
TaskListPanel::Prepare(ContainerWindow &parent, const PixelRect &rc)
{
  LoadWindow(task_list_callbacks, parent, rc,
             _T("IDR_XML_TASKLIST"));

  instance = this;

  task_store = new TaskStore();
  lazy_loaded = false;

  // Save important control pointers
  wTasks = (ListControl*)form.FindByName(_T("frmTasks"));
  assert(wTasks != NULL);
  wTasks->SetHandler(this);

  more_button = (WndButton *)form.FindByName(_T("more"));
  assert(more_button != NULL);
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
  if (!lazy_loaded) {
    lazy_loaded = true;
    // Scan XCSoarData for available tasks
    task_store->Scan(more);
  }

  if (wTaskView != NULL) {
    wTaskView->SetOnPaintNotify(::OnTaskPaint);
    wTaskView->Show();
  }

  wTasks->SetCursorIndex(0); // so Save & Declare are always available
  RefreshView();
  XMLWidget::Show(rc);
}

void
TaskListPanel::Hide()
{
  if (wTaskView != NULL)
    dlgTaskManager::ResetTaskView(wTaskView);

  XMLWidget::Hide();
}
