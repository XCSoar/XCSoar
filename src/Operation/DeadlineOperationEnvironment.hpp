// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The XCSoar Project

#pragma once

#include "ProxyOperationEnvironment.hpp"

#include <chrono>

/**
 * An #OperationEnvironment that reports itself cancelled once a deadline
 * passes, as well as when the user cancels.
 *
 * Lets an operation carry a hard time budget without knowing anything about
 * clocks: to the operation this is indistinguishable from the user pressing
 * Cancel, so whatever it does with a partial result it does with this too.
 */
class DeadlineOperationEnvironment final : public ProxyOperationEnvironment {
  const std::chrono::steady_clock::time_point deadline;

public:
  DeadlineOperationEnvironment(OperationEnvironment &_other,
                               std::chrono::steady_clock::duration budget)
    noexcept
    :ProxyOperationEnvironment(_other),
     deadline(std::chrono::steady_clock::now() + budget) {}

  /* deliberately not [[gnu::pure]]: this reads the clock, so its value
     depends on more than its arguments and the compiler must not cache it */
  bool IsExpired() const noexcept {
    return std::chrono::steady_clock::now() >= deadline;
  }

  /* virtual methods from class OperationEnvironment */
  bool IsCancelled() const noexcept override {
    return IsExpired() || ProxyOperationEnvironment::IsCancelled();
  }
};
