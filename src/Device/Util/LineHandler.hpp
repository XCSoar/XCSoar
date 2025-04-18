// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The XCSoar Project

#pragma once

class PortLineHandler {
public:
  virtual bool LineReceived(const char *line) noexcept = 0;
};
