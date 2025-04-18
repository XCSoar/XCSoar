// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The XCSoar Project

#pragma once

#include "ProxyOperationEnvironment.hpp"

/**
 * An #OperationEnvironment implementation that is not cancellable,
 * even if the underlying object is.
 */
class NoCancelOperationEnvironment : public ProxyOperationEnvironment {
public:
  NoCancelOperationEnvironment(OperationEnvironment &_other) noexcept
    :ProxyOperationEnvironment(_other) {}

  /* virtual methods from class OperationEnvironment */
  bool IsCancelled() const noexcept override;
  void SetCancelHandler(std::function<void()> handler) noexcept override;
  void Sleep(std::chrono::steady_clock::duration duration) noexcept override;
};
