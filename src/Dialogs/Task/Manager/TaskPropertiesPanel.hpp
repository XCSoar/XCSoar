// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The XCSoar Project

#pragma once

#include "Widget/RowFormWidget.hpp"
#include "Form/DataField/Listener.hpp"
#include "Engine/Task/Factory/TaskFactoryType.hpp"

class TaskManagerDialog;
class OrderedTask;
class DataFieldBoolean;
class DataFieldEnum;

class TaskPropertiesPanel final
  : public RowFormWidget,
    private DataFieldListener {

  TaskManagerDialog &dialog;

  std::unique_ptr<OrderedTask> &ordered_task_pointer;
  OrderedTask *ordered_task;
  bool *task_changed;

  TaskFactoryType orig_taskType;

public:
  TaskPropertiesPanel(TaskManagerDialog &_dialog,
                      std::unique_ptr<OrderedTask> &_active_task,
                      bool *_task_modified) noexcept;

  void OnFAIFinishHeightChange(DataFieldBoolean &df);
  void OnTaskTypeChange(DataFieldEnum &df);

  /* virtual methods from Widget */
  void Prepare(ContainerWindow &parent, const PixelRect &rc) noexcept override;
  void ReClick() noexcept override;
  void Show(const PixelRect &rc) noexcept override;

  /**
   * Saves the panel's properties of the task to the temporary task
   * so they can be used by other panels editing the task.
   * @return true
   */
  bool Leave() noexcept override;

protected:
  void RefreshView();
  void ReadValues();

private:
  /* virtual methods from DataFieldListener */
  void OnModified(DataField &df) noexcept override;
};
