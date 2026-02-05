// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The XCSoar Project

#pragma once

#include "Operation.hpp"

/**
 * An #OperationEnvironment implementation that forwards all calls to
 * another #OperationEnvironment instance.  Similar to
 * #ProxyOperationEnvironment, but this one's underlying instance is
 * optional.
 */
class PluggableOperationEnvironment final : public OperationEnvironment {
  OperationEnvironment *other = nullptr;

public:
  void SetOperationEnvironment(OperationEnvironment &_other) noexcept {
    other = &_other;
  }

  void SetOperationEnvironment(std::nullptr_t n) noexcept {
    other = n;
  }

  /* virtual methods from class OperationEnvironment */
  bool IsCancelled() const noexcept override;
  void SetCancelHandler(std::function<void()> handler) noexcept override;
  void Sleep(std::chrono::steady_clock::duration duration) noexcept override;
  void SetErrorMessage(const char *text) noexcept override;
  void SetText(const char *text) noexcept override;
  void SetProgressRange(unsigned range) noexcept override;
  void SetProgressPosition(unsigned position) noexcept override;
};
