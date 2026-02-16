// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The XCSoar Project

#pragma once

#include "Form/Form.hpp"
#include "Form/Button.hpp"
#include "Operation/Operation.hpp"
#include "ProgressWindow.hpp"

#include <functional>

class ProgressDialog
  : public WndForm, public QuietOperationEnvironment {

  ProgressWindow progress;

  Button cancel_button;
  std::function<void()> cancel_callback;

public:
  ProgressDialog(UI::SingleWindow &parent, const DialogLook &dialog_look,
                 const char *caption);

  void AddCancelButton(std::function<void()> &&callback={});

  /* virtual methods from class OperationEnvironment */

  void SetText(const char *text) noexcept override {
    progress.SetMessage(text);
  }

  void SetProgressRange(unsigned range) noexcept override {
    progress.SetRange(0, range);
  }

  void SetProgressPosition(unsigned position) noexcept override {
    progress.SetValue(position);
  }

  /* virtual methods from WndForm */
  void SetModalResult(int id) noexcept override;
};
