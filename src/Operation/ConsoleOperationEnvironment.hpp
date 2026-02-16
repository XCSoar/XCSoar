// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The XCSoar Project

#pragma once

#include "Operation/Operation.hpp"

class ConsoleOperationEnvironment : public QuietOperationEnvironment {
  unsigned range;

public:
  /* virtual methods from class OperationEnvironment */
  void SetErrorMessage(const char *text) noexcept override;
  void SetText(const char *text) noexcept override;
  void SetProgressRange(unsigned range) noexcept override;
  void SetProgressPosition(unsigned position) noexcept override;
};
