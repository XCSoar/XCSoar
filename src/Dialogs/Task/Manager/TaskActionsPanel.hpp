// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The XCSoar Project

#pragma once

#include "Widget/RowFormWidget.hpp"

class TaskManagerDialog;
class TaskMiscPanel;
class OrderedTask;

class TaskActionsPanel : public RowFormWidget {
  enum Controls {
    NEW_TASK,
    DECLARE,
    BROWSE,
    SAVE,
  };

  TaskManagerDialog &dialog;
  TaskMiscPanel &parent;

  std::unique_ptr<OrderedTask> &active_task;
  bool *task_modified;

public:
  TaskActionsPanel(TaskManagerDialog &_dialog, TaskMiscPanel &_parent,
                   std::unique_ptr<OrderedTask> &_active_task,
                   bool *_task_modified) noexcept;

private:
  void SaveTask();

  void OnBrowseClicked();
  void OnNewTaskClicked();
  void OnDeclareClicked();
  void OnDownloadClicked() noexcept;

  /* virtual methods from class Widget */
  void Prepare(ContainerWindow &parent, const PixelRect &rc) noexcept override;
  void ReClick() noexcept override;
};
