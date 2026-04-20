// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The XCSoar Project

#pragma once

/**
 * Callback interface for #TabDisplay.
 */
class TabHandler {
public:
  virtual bool ClickPage(unsigned i) noexcept = 0;
  virtual bool PreviousPage() noexcept = 0;
  virtual bool NextPage() noexcept = 0;
};
