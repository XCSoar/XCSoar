// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The XCSoar Project

#include "TaskListPanel.hpp"
#include "Internal.hpp"
#include "../dlgTaskHelpers.hpp"
#include "Dialogs/Message.hpp"
#include "Dialogs/TextEntry.hpp"
#include "Form/Button.hpp"
#include "Form/ButtonPanel.hpp"
#include "Widget/ListWidget.hpp"
#include "Widget/TextWidget.hpp"
#include "Widget/ButtonPanelWidget.hpp"
#include "Widget/TwoWidgets.hpp"
#include "Task/TaskStore.hpp"
#include "Task/ValidationErrorStrings.hpp"
#include "LocalPath.hpp"
#include "system/FileUtil.hpp"
#include "Language/Language.hpp"
#include "Interface.hpp"
#include "Renderer/TextRowRenderer.hpp"
#include "Look/DialogLook.hpp"
#include "Engine/Task/Ordered/OrderedTask.hpp"
#include "util/StringCompare.hxx"
#include "UIGlobals.hpp"
#include "Components.hpp" // for way_points
#include "DataComponents.hpp"

#include <cassert>

static unsigned task_list_serial;

/* this macro exists in the WIN32 API */
#ifdef DELETE
#undef DELETE
#endif

class TaskListPanel final
  : public ListWidget {

  TaskManagerDialog &dialog;

  TextRowRenderer row_renderer;

  std::unique_ptr<OrderedTask> &active_task;
  bool *task_modified;

  TaskStore task_store;
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
                std::unique_ptr<OrderedTask> &_active_task, bool *_task_modified,
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
    buttons.Add(_("Load"), [this](){ LoadTask(); });
    buttons.Add(_("Rename"), [this](){ RenameTask(); });
    buttons.Add(_("Delete"), [this](){ DeleteTask(); });
    more_button = buttons.Add(_("More"), [this](){ OnMoreClicked(); });
  }

  void RefreshView();

  void LoadTask();
  void DeleteTask();
  void RenameTask();

  void OnMoreClicked();

  void Prepare(ContainerWindow &parent, const PixelRect &rc) noexcept override;
  void Show(const PixelRect &rc) noexcept override;
  void Hide() noexcept override;

protected:
  const OrderedTask *get_cursor_task();

  [[gnu::pure]]
  const char *get_cursor_name();

private:
  /* virtual methods from class ListControl::Handler */
  void OnPaintItem([[maybe_unused]] Canvas &canvas, [[maybe_unused]] const PixelRect rc,
                   [[maybe_unused]] unsigned idx) noexcept override;

  void OnCursorMoved([[maybe_unused]] unsigned index) noexcept override {
    RefreshView();
  }

  bool CanActivateItem([[maybe_unused]] unsigned index) const noexcept override {
      return true;
  }

  void OnActivateItem([[maybe_unused]] unsigned index) noexcept override {
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
  if (cursor_index >= task_store.Size())
    return nullptr;

  const OrderedTask *ordered_task =
    task_store.GetTask(cursor_index,
                       CommonInterface::GetComputerSettings().task,
                       data_components->waypoints.get());

  if (ordered_task == nullptr)
    return nullptr;

  return ordered_task;
}

const char *
TaskListPanel::get_cursor_name()
{
  const unsigned cursor_index = GetList().GetCursorIndex();
  if (cursor_index >= task_store.Size())
    return "";

  return task_store.GetName(cursor_index);
}

void
TaskListPanel::OnPaintItem(Canvas &canvas, const PixelRect rc,
                           unsigned DrawListIndex) noexcept
{
  assert(DrawListIndex <= task_store.Size());

  row_renderer.DrawTextRow(canvas, rc, task_store.GetName(DrawListIndex));
}

void
TaskListPanel::RefreshView()
{
  GetList().SetLength(task_store.Size());

  dialog.InvalidateTaskView();

  const OrderedTask *ordered_task = get_cursor_task();
  dialog.ShowTaskView(ordered_task);

  if (ordered_task == nullptr) {
    summary.SetText("");
  } else {
    char text[300];
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
  text.Format("%s\n(%s)", _("Load the selected task?"),
              get_cursor_name());

  if (const auto errors = orig->CheckTask(); !errors.IsEmpty()) {
    text.append("\n");
    text.append(getTaskValidationErrors(errors));
  }

  if (ShowMessageBox(text.c_str(), _("Task Browser"),
                  MB_YESNO | MB_ICONQUESTION) != IDYES)
    return;

  // create new task first to guarantee pointers are different
  active_task = orig->Clone(CommonInterface::GetComputerSettings().task);

  const unsigned cursor_index = GetList().GetCursorIndex();
  active_task->SetName(task_store.GetName(cursor_index));

  RefreshView();
  *task_modified = true;

  dialog.SwitchToEditTab();
}

void
TaskListPanel::DeleteTask()
{
  const unsigned cursor_index = GetList().GetCursorIndex();
  if (cursor_index >= task_store.Size())
    return;

  const auto path = task_store.GetPath(cursor_index);
  if (StringEndsWithIgnoreCase(path.c_str(), ".cup")) {
    ShowMessageBox(_("Can't delete .CUP files"), _("Error"),
                   MB_OK | MB_ICONEXCLAMATION);
    return;
  }

  const char *fname = task_store.GetName(cursor_index);

  StaticString<1024> text;
  text.Format("%s\n(%s)", _("Delete the selected task?"), fname);
  if (ShowMessageBox(text.c_str(), _("Task Browser"),
                  MB_YESNO | MB_ICONQUESTION) != IDYES)
    return;

  File::Delete(path);

  task_store.Scan(more);
  RefreshView();
}

static bool
ClearSuffix(char *p, const char *suffix)
{
  size_t length = strlen(p);
  size_t suffix_length = strlen(suffix);
  if (length <= suffix_length)
    return false;

  char *q = p + length - suffix_length;
  if (!StringIsEqualIgnoreCase(q, suffix))
    return false;

  *q = '\0';
  return true;
}

void
TaskListPanel::RenameTask()
{
  const unsigned cursor_index = GetList().GetCursorIndex();
  if (cursor_index >= task_store.Size())
    return;

  const char *oldname = task_store.GetName(cursor_index);
  StaticString<40> newname(oldname);

  if (ClearSuffix(newname.buffer(), ".cup")) {
    ShowMessageBox(_("Can't rename .CUP files"), _("Rename Error"),
        MB_ICONEXCLAMATION);
    return;
  }

  ClearSuffix(newname.buffer(), ".tsk");

  if (!TextEntryDialog(newname))
    return;

  newname.append(".tsk");

  const auto tasks_path = MakeLocalPath("tasks");

  File::Rename(task_store.GetPath(cursor_index),
               AllocatedPath::Build(tasks_path, newname));

  task_store.Scan(more);
  RefreshView();
}

void
TaskListPanel::OnMoreClicked()
{
  more = !more;

  more_button->SetCaption(more ? _("Less") : _("More"));

  task_store.Scan(more);
  RefreshView();
}

void
TaskListPanel::Prepare(ContainerWindow &parent, const PixelRect &rc) noexcept
{
  const DialogLook &look = UIGlobals::GetDialogLook();

  CreateList(parent, dialog.GetLook(), rc,
             row_renderer.CalculateLayout(*look.list.font));

  CreateButtons(buttons->GetButtonPanel());

  /* mark the new TaskStore as "dirty" until the data directory really
     gets scanned */
  serial = task_list_serial - 1;
}

void
TaskListPanel::Show(const PixelRect &rc) noexcept
{
  if (serial != task_list_serial) {
    serial = task_list_serial;
    // Scan XCSoarData for available tasks
    task_store.Scan(more);
  }

  dialog.ShowTaskView(get_cursor_task());

  GetList().SetCursorIndex(0); // so Save & Declare are always available
  RefreshView();
  ListWidget::Show(rc);
}

void
TaskListPanel::Hide() noexcept
{
  dialog.ResetTaskView();

  ListWidget::Hide();
}

std::unique_ptr<Widget>
CreateTaskListPanel(TaskManagerDialog &dialog,
                    std::unique_ptr<OrderedTask> &active_task,
                    bool *task_modified) noexcept
{
  auto summary = std::make_unique<TextWidget>();
  auto widget = std::make_unique<TaskListPanel>(dialog, active_task, task_modified,
                                                *summary);
  auto tw = std::make_unique<TwoWidgets>(std::move(widget),
                                         std::move(summary));
  auto &list = (TaskListPanel &)tw->GetFirst();

  list.SetTwoWidgets(*tw);

  auto buttons =
    std::make_unique<ButtonPanelWidget>(std::move(tw),
                                        ButtonPanelWidget::Alignment::BOTTOM);
  list.SetButtonPanel(*buttons);

  return buttons;
}

void
DirtyTaskListPanel()
{
  ++task_list_serial;
}
