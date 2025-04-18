// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The XCSoar Project

#pragma once

#include "Widget/Widget.hpp"
#include "Form/Frame.hpp"
#include "Form/Button.hpp"

class TaskManagerDialog;

class TaskClosePanel final : public NullWidget {
  enum Buttons {
    CLOSE,
    REVERT,
  };

  struct Layout {
    PixelRect close_button, message, revert_button;

    Layout(PixelRect rc, const DialogLook &look) noexcept;
  };

public:
  TaskManagerDialog &dialog;

private:
  bool *task_modified;

  const DialogLook &look;

  Button close_button;
  WndFrame message;
  Button revert_button;

public:
  TaskClosePanel(TaskManagerDialog &_dialog, bool *_task_modified,
                 const DialogLook &_look) noexcept;

  void CommitAndClose() noexcept;
  void RefreshStatus() noexcept;

  void Prepare(ContainerWindow &parent, const PixelRect &rc) noexcept override;
  bool Click() noexcept override;
  void ReClick() noexcept override;
  void Show(const PixelRect &rc) noexcept override;
  void Hide() noexcept override;
  void Move(const PixelRect &rc) noexcept override;
  bool SetFocus() noexcept override;
  bool HasFocus() const noexcept override;
};
