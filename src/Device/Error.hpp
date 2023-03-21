// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The XCSoar Project

#pragma once

#include <stdexcept>

class DeviceTimeout final : public std::runtime_error {
public:
  using std::runtime_error::runtime_error;
};
