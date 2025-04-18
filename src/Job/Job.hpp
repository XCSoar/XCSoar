// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The XCSoar Project

#pragma once

class OperationEnvironment;

/**
 * Base class for a job that can be offloaded into a separate thread.
 */
class Job {
public:
  virtual void Run(OperationEnvironment &env) = 0;
};
