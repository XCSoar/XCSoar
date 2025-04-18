// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The XCSoar Project

#pragma once

class Job;

/**
 * An environment that can run one or more jobs.
 */
class JobRunner {
public:
  /**
   * @return true if the job has finished (may have failed), false if
   * the job was cancelled by the user
   */
  virtual bool Run(Job &job) = 0;
};
