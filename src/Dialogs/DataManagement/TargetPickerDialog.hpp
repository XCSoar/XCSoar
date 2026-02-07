// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The XCSoar Project

#pragma once

#include "LocalPath.hpp"

namespace UI { class SingleWindow; }
class DialogLook;

class TargetPickerDialog {
public:
  TargetPickerDialog(UI::SingleWindow &parent, const DialogLook &look) noexcept;

  // Show the dialog modally and return the chosen target path or nullptr
  AllocatedPath ShowModal() noexcept;

  // Return last selected target (ensures a local default exists)
  static const AllocatedPath &GetLastTarget() noexcept;

  // Return the first available target device, or nullptr if none
  static AllocatedPath GetDefaultTarget() noexcept;

  // Set last selected target
  static void SetLastTarget(const AllocatedPath &p) noexcept;

private:
  UI::SingleWindow &parent_window;
  const DialogLook &dialog_look;
};
