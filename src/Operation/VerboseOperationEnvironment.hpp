// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The XCSoar Project

#pragma once

#include "MessageOperationEnvironment.hpp"

class VerboseOperationEnvironment : public MessageOperationEnvironment {
public:
  /**
   * Call after the main window has been resized.
   */
  void UpdateLayout() noexcept;

  void Hide() noexcept;

  /* virtual methods from class OperationEnvironment */
  void SetText(const char *text) noexcept override;
  void SetProgressRange(unsigned range) noexcept override;
  void SetProgressPosition(unsigned position) noexcept override;
};
