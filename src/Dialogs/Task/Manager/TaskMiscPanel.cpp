// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The XCSoar Project

#include "TaskMiscPanel.hpp"
#include "TaskActionsPanel.hpp"
#include "TaskListPanel.hpp"
#include "WeGlideTasksPanel.hpp"

TaskMiscPanel::TaskMiscPanel(TaskManagerDialog &dialog,
                             std::unique_ptr<OrderedTask> &_active_task,
                             bool *_task_modified) noexcept
{
  Add(std::make_unique<TaskActionsPanel>(dialog, *this, _active_task, _task_modified));

  Add(CreateTaskListPanel(dialog, _active_task, _task_modified));
  Add(CreateWeGlideTasksPanel(dialog, WeGlideTaskSelection::USER,
                              _active_task, _task_modified));
  Add(CreateWeGlideTasksPanel(dialog, WeGlideTaskSelection::PUBLIC_DECLARED,
                              _active_task, _task_modified));
}

void
TaskMiscPanel::ReClick() noexcept
{
  if (GetCurrentIndex() > 0)
    SetCurrent(PAGE_ACTIONS);
  else
    PagerWidget::ReClick();
}

void
TaskMiscPanel::Show(const PixelRect &rc) noexcept
{
  SetCurrent(PAGE_ACTIONS);
  PagerWidget::Show(rc);
}
