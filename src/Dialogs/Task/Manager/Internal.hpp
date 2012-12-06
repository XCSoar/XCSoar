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

#ifndef XCSOAR_TASK_MANAGER_INTERNAL_HPP
#define XCSOAR_TASK_MANAGER_INTERNAL_HPP

#include "Form/Form.hpp"

class WndOwnerDrawFrame;
class WndButton;
class TabBarControl;
class OrderedTask;

class TaskManagerDialog : public WndForm {
  PixelRect task_view_position;

  WndOwnerDrawFrame *task_view;
  WndButton *target_button;
  TabBarControl *tab_bar;

  OrderedTask *task;

  bool fullscreen;

  bool modified;

public:
  TaskManagerDialog(const DialogLook &look)
    :WndForm(look),
     task_view(nullptr), target_button(nullptr), tab_bar(nullptr),
     task(nullptr),
     fullscreen(false), modified(false) {}

  virtual ~TaskManagerDialog();

  const OrderedTask &GetTask() const {
    return *task;
  }

  void Create(SingleWindow &parent);
  void Destroy();

  void UpdateCaption();

  void InvalidateTaskView();
  void TaskViewClicked();
  void RestoreTaskView();
  void ShowTaskView();
  void ShowTaskView(void (*paint)(WndOwnerDrawFrame *sender, Canvas &canvas));
  void ResetTaskView();

  void SwitchToEditTab();
  void SwitchToPropertiesPanel();

  /**
   * Validates task and prompts if change or error
   * Commits task if no error
   * @return True if task manager should close
   *         False if window should remain open
   */
  bool Commit();

  void Revert();

  /* virtual methods from class ActionListener */
  virtual void OnAction(int id) gcc_override;
};

#endif /* DLGTASKMANAGER_HPP */
