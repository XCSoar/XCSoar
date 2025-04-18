// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The XCSoar Project

#pragma once

#include "DataHandler.hpp"

class NullDataHandler : public DataHandler {
public:
  bool DataReceived(std::span<const std::byte>) noexcept override {
    return true;
  }
};
