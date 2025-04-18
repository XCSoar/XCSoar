// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The XCSoar Project

#pragma once

class DataField;

class DataFieldListener {
public:
  virtual void OnModified(DataField &df) noexcept = 0;
};
