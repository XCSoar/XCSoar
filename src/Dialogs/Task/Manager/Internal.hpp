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

#ifndef XCSOAR_TASK_MANAGER_INTERNAL_HPP
#define XCSOAR_TASK_MANAGER_INTERNAL_HPP

#include "Widget/TabWidget.hpp"
#include "Form/Form.hpp"

#include <memory>

class OrderedTask;
class ButtonWidget;

class TaskManagerDialog final : public TabWidget {
  enum Tabs {
    TurnpointTab,
    PropertiesTab,
    RulesTab,
    CloseTab,
  };

  WndForm &dialog;

  std::unique_ptr<OrderedTask> task;

  bool fullscreen = false;

  bool modified = false;

public:
  explicit TaskManagerDialog(WndForm &_dialog) noexcept;

  ~TaskManagerDialog() noexcept override;

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

  void Create(UI::SingleWindow &parent);
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
    ShowTaskView(task.get());
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
  void Initialise(ContainerWindow &parent, const PixelRect &rc) noexcept override;
  void Show(const PixelRect &rc) noexcept override;
  bool KeyPress(unsigned key_code) noexcept override;

protected:
  /* virtual methods from class PagerWidget */
  void OnPageFlipped() noexcept override;
};

#endif /* DLGTASKMANAGER_HPP */
