// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The XCSoar Project

#pragma once

/**
 * Receive progress updates.
 */
class ProgressListener {
public:
  /**
   * Initialize the progress bar, and set the maximum value which will
   * mean "100% done".  The default value is 0, which means "no
   * progress bar".
   */
  virtual void SetProgressRange(unsigned range) noexcept = 0;

  /**
   * Set the current position of the progress bar.  Must not be bigger
   * than the configured range.
   */
  virtual void SetProgressPosition(unsigned position) noexcept = 0;
};
