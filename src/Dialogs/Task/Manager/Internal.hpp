// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The XCSoar Project

#pragma once

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

  bool modified = true;

public:
  TaskManagerDialog(WndForm &_dialog,
                    std::unique_ptr<OrderedTask> &&_task) noexcept;

  ~TaskManagerDialog() noexcept override;

  const DialogLook &GetLook() const {
    return dialog.GetLook();
  }

  auto &GetMainWindow() const noexcept {
    return dialog.GetMainWindow();
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
