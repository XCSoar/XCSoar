// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The XCSoar Project

#pragma once

#include "ProxyOperationEnvironment.hpp"

/**
 * An #OperationEnvironment implementation which translates progress
 * values.
 */
class SubOperationEnvironment : public ProxyOperationEnvironment {
  const unsigned parent_start, parent_range;

  unsigned range = 0;

public:
  SubOperationEnvironment(OperationEnvironment &_other,
                          unsigned _start, unsigned _end) noexcept
    :ProxyOperationEnvironment(_other),
     parent_start(_start), parent_range(_end - _start) {}

  /* virtual methods from class ProgressListener */
  void SetProgressRange(unsigned range) noexcept override;
  void SetProgressPosition(unsigned position) noexcept override;
};
