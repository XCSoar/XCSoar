/*
Copyright_License {

  XCSoar Glide Computer - http://www.xcsoar.org/
  Copyright (C) 2000-2021 The XCSoar Project
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

#ifndef XCSOAR_TASK_ACTIONS_PANEL_HPP
#define XCSOAR_TASK_ACTIONS_PANEL_HPP

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

  /* virtual methods from class Widget */
  void Prepare(ContainerWindow &parent, const PixelRect &rc) override;
  void ReClick() override;
};

#endif
