// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The XCSoar Project

#pragma once

#include "Operation.hpp"

/**
 * A #OperationEnvironment implementation that displays error messages
 * with ShowMessageBox().
 */
class MessageOperationEnvironment : public QuietOperationEnvironment {
public:
  /* virtual methods from class OperationEnvironment */
  void SetErrorMessage(const TCHAR *text) noexcept override;
};
