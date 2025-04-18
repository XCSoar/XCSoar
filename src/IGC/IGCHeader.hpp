// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The XCSoar Project

#pragma once

struct IGCHeader {
  /**
   * 3-letter manufacturer id.
   */
  char manufacturer[4];

  /**
   * 3-letter logger id.
   */
  char id[4];

  /**
   * The flight number on that day.
   */
  unsigned flight;
};
