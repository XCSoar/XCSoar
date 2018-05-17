/*
Copyright_License {

  XCSoar Glide Computer - http://www.xcsoar.org/
  Copyright (C) 2000-2016 The XCSoar Project
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
#include "../dlgTaskHelpers.hpp"
#include "Dialogs/Message.hpp"
#include "Dialogs/TextEntry.hpp"
#include "Form/Button.hpp"
#include "Form/ButtonPanel.hpp"
#include "Form/List.hpp"
#include "Widget/ListWidget.hpp"
#include "Widget/TextWidget.hpp"
#include "Widget/ButtonPanelWidget.hpp"
#include "Widget/TwoWidgets.hpp"
#include "Task/TaskStore.hpp"
#include "LocalPath.hpp"
#include "OS/FileUtil.hpp"
#include "Language/Language.hpp"
#include "Interface.hpp"
#include "Screen/Canvas.hpp"
#include "Screen/Layout.hpp"
#include "Engine/Task/Ordered/OrderedTask.hpp"
#include "Util/StringCompare.hxx"

#include <assert.h>

static unsigned task_list_serial;

/* this macro exists in the WIN32 API */
#ifdef DELETE
#undef DELETE
#endif

class TaskListPanel final
  : public ListWidget, private ActionListener {
  enum Buttons {
    LOAD = 100,
    RENAME,
    DELETE,
    MORE,
  };

  TaskManagerDialog &dialog;

  OrderedTask **active_task;
  bool *task_modified;

  TaskStore *task_store;
  unsigned serial;

  /**
   * Showing all task files?  (including *.igc, *.cup)
   */
  bool more;

  Button *more_button;
  TextWidget &summary;
  TwoWidgets *two_widgets;
  ButtonPanelWidget *buttons;

public:
  TaskListPanel(TaskManagerDialog &_dialog,
                OrderedTask **_active_task, bool *_task_modified,
                TextWidget &_summary)
    :dialog(_dialog),
     active_task(_active_task), task_modified(_task_modified),
     more(false),
     summary(_summary)  {}

  void SetTwoWidgets(TwoWidgets &_two_widgets) {
    two_widgets = &_two_widgets;
  }

  void SetButtonPanel(ButtonPanelWidget &_buttons) {
    buttons = &_buttons;
  }

  void CreateButtons(ButtonPanel &buttons) {
    buttons.Add(_("Load"), *this, LOAD);
    buttons.Add(_("Rename"), *this, RENAME);
    buttons.Add(_("Delete"), *this, DELETE);
    more_button = buttons.Add(_("More"), *this, MORE);
  }

  void RefreshView();

  void LoadTask();
  void DeleteTask();
  void RenameTask();

  void OnMoreClicked();

  void Prepare(ContainerWindow &parent, const PixelRect &rc) override;
  void Unprepare() override;
  void Show(const PixelRect &rc) override;
  void Hide() override;

protected:
  const OrderedTask *get_cursor_task();

  gcc_pure
  const TCHAR *get_cursor_name();

private:
  /* virtual methods from ActionListener */
  void OnAction(int id) override;

  /* virtual methods from class ListControl::Handler */
  void OnPaintItem(Canvas &canvas, const PixelRect rc, unsigned idx) override;

  void OnCursorMoved(unsigned index) override {
    RefreshView();
  }

  bool CanActivateItem(unsigned index) const override {
      return true;
  }

  void OnActivateItem(unsigned index) override {
    LoadTask();
  }
};

/**
 * used for browsing saved tasks
 * must be valid (2 task points)
 * @return nullptr if no valid task at cursor, else pointer to task;
 */
const OrderedTask *
TaskListPanel::get_cursor_task()
{
  const unsigned cursor_index = GetList().GetCursorIndex();
  if (cursor_index >= task_store->Size())
    return nullptr;

  const OrderedTask *ordered_task =
    task_store->GetTask(cursor_index,
                        CommonInterface::GetComputerSettings().task);

  if (ordered_task == nullptr || !ordered_task->CheckTask())
    return nullptr;

  return ordered_task;
}

const TCHAR *
TaskListPanel::get_cursor_name()
{
  const unsigned cursor_index = GetList().GetCursorIndex();
  if (cursor_index >= task_store->Size())
    return _T("");

  return task_store->GetName(cursor_index);
}

void
TaskListPanel::OnAction(int id)
{
  switch (id) {
  case LOAD:
    LoadTask();
    break;

  case RENAME:
    RenameTask();
    break;

  case DELETE:
    DeleteTask();
    break;

  case MORE:
    OnMoreClicked();
    break;
  }
}

void
TaskListPanel::OnPaintItem(Canvas &canvas, const PixelRect rc,
                           unsigned DrawListIndex)
{
  assert(DrawListIndex <= task_store->Size());

  const unsigned padding = Layout::GetTextPadding();
  const TCHAR *name = task_store->GetName(DrawListIndex);

  canvas.DrawText(rc.left + padding, rc.top + padding, name);
}

void
TaskListPanel::RefreshView()
{
  GetList().SetLength(task_store->Size());

  dialog.InvalidateTaskView();

  const OrderedTask *ordered_task = get_cursor_task();
  dialog.ShowTaskView(ordered_task);

  if (ordered_task == nullptr) {
    summary.SetText(_T(""));
  } else {
    TCHAR text[300];
    OrderedTaskSummary(ordered_task, text, false);
    summary.SetText(text);
  }

  if (GetList().IsVisible() && two_widgets != nullptr)
    two_widgets->UpdateLayout();
}

void
TaskListPanel::LoadTask()
{
  const OrderedTask* orig = get_cursor_task();
  if (orig == nullptr)
    return;

  StaticString<1024> text;
  text.Format(_T("%s\n(%s)"), _("Load the selected task?"),
              get_cursor_name());

  if (ShowMessageBox(text.c_str(), _("Task Browser"),
                  MB_YESNO | MB_ICONQUESTION) != IDYES)
    return;

  // create new task first to guarantee pointers are different
  OrderedTask* temptask = orig->Clone(CommonInterface::GetComputerSettings().task);
  delete *active_task;
  *active_task = temptask;

  const unsigned cursor_index = GetList().GetCursorIndex();
  (*active_task)->SetName(StaticString<64>(task_store->GetName(cursor_index)));

  RefreshView();
  *task_modified = true;

  dialog.SwitchToEditTab();
}

void
TaskListPanel::DeleteTask()
{
  const unsigned cursor_index = GetList().GetCursorIndex();
  if (cursor_index >= task_store->Size())
    return;

  const auto path = task_store->GetPath(cursor_index);
  if (StringEndsWithIgnoreCase(path.c_str(), _T(".cup"))) {
    ShowMessageBox(_("Can't delete .CUP files"), _("Error"),
                   MB_OK | MB_ICONEXCLAMATION);
    return;
  }

  const TCHAR *fname = task_store->GetName(cursor_index);

  StaticString<1024> text;
  text.Format(_T("%s\n(%s)"), _("Delete the selected task?"), fname);
  if (ShowMessageBox(text.c_str(), _("Task Browser"),
                  MB_YESNO | MB_ICONQUESTION) != IDYES)
    return;

  File::Delete(path);

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
  const unsigned cursor_index = GetList().GetCursorIndex();
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

  const auto tasks_path = MakeLocalPath(_T("tasks"));

  File::Rename(task_store->GetPath(cursor_index),
               AllocatedPath::Build(tasks_path, newname));

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

void
TaskListPanel::Prepare(ContainerWindow &parent, const PixelRect &rc)
{
  CreateList(parent, dialog.GetLook(),
             rc, Layout::GetMinimumControlHeight());

  CreateButtons(buttons->GetButtonPanel());

  task_store = new TaskStore();

  /* mark the new TaskStore as "dirty" until the data directory really
     gets scanned */
  serial = task_list_serial - 1;
}

void
TaskListPanel::Unprepare()
{
  delete task_store;
  DeleteWindow();
}

void
TaskListPanel::Show(const PixelRect &rc)
{
  if (serial != task_list_serial) {
    serial = task_list_serial;
    // Scan XCSoarData for available tasks
    task_store->Scan(more);
  }

  dialog.ShowTaskView(get_cursor_task());

  GetList().SetCursorIndex(0); // so Save & Declare are always available
  RefreshView();
  ListWidget::Show(rc);
}

void
TaskListPanel::Hide()
{
  dialog.ResetTaskView();

  ListWidget::Hide();
}

Widget *
CreateTaskListPanel(TaskManagerDialog &dialog,
                    OrderedTask **active_task, bool *task_modified)
{
  TextWidget *summary = new TextWidget();
  TaskListPanel *widget = new TaskListPanel(dialog, active_task, task_modified,
                                            *summary);
  TwoWidgets *tw = new TwoWidgets(widget, summary);
  widget->SetTwoWidgets(*tw);

  ButtonPanelWidget *buttons =
    new ButtonPanelWidget(tw, ButtonPanelWidget::Alignment::BOTTOM);
  widget->SetButtonPanel(*buttons);

  return buttons;
}

void
DirtyTaskListPanel()
{
  ++task_list_serial;
}
