// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The XCSoar Project

#pragma once

#include "system/Path.hpp"

struct DialogLook;

class TargetPickerDialog {
  const DialogLook &dialog_look;

public:
  explicit TargetPickerDialog(const DialogLook &look) noexcept;

  /** Show the dialog modally; returns the chosen path, or a null path on cancel. */
  AllocatedPath ShowModal() noexcept;

  /** Return last selected target (ensures a local default exists). */
  static const AllocatedPath &GetLastTarget() noexcept;

  /** Return the first available target device, or a null path if none. */
  static AllocatedPath GetDefaultTarget() noexcept;

  /** Set last selected target. */
  static void SetLastTarget(const AllocatedPath &p) noexcept;

private:
  static AllocatedPath last_target;
};
