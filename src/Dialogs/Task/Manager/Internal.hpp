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

#ifndef XCSOAR_TASK_MANAGER_INTERNAL_HPP
#define XCSOAR_TASK_MANAGER_INTERNAL_HPP

#include "Widget/TabWidget.hpp"
#include "Form/Form.hpp"

class OrderedTask;
class ButtonWidget;

class TaskManagerDialog final : public TabWidget, ActionListener {
  enum Tabs {
    TurnpointTab,
    PropertiesTab,
    RulesTab,
    CloseTab,
  };

  WndForm &dialog;

  OrderedTask *task;

  bool fullscreen;

  bool modified;

public:
  explicit TaskManagerDialog(WndForm &_dialog)
    :TabWidget(Orientation::AUTO),
     dialog(_dialog),
     task(nullptr),
     fullscreen(false), modified(false) {}

  virtual ~TaskManagerDialog();

  const DialogLook &GetLook() const {
    return dialog.GetLook();
  }

  void FocusFirstControl() {
    dialog.FocusFirstControl();
  }

  void SetModalResult(int r) {
    dialog.SetModalResult(r);
  }

  const OrderedTask &GetTask() const {
    return *task;
  }

  void Create(SingleWindow &parent);
  void Destroy();

  void UpdateCaption();

  void InvalidateTaskView();

  void TaskViewClicked() {
    ToggleLargeExtra();
  }

  void RestoreTaskView() {
    RestoreExtra();
  }

  void ShowTaskView(const OrderedTask *task);

  void ResetTaskView() {
      ShowTaskView(task);
  }

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

  /* virtual methods from class Widget */
  void Initialise(ContainerWindow &parent, const PixelRect &rc) override;
  void Show(const PixelRect &rc) override;
  bool KeyPress(unsigned key_code) override;

protected:
  /* virtual methods from class PagerWidget */
  void OnPageFlipped() override;

private:
  /* virtual methods from class ActionListener */
  void OnAction(int id) override;
};

#endif /* DLGTASKMANAGER_HPP */
