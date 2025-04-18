// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The XCSoar Project

#pragma once

#include <cstddef>
#include <span>

/**
 * Interface with callbacks for the #Port class.
 */
class DataHandler {
public:
  /**
   * @return false if the handler wishes to receive no more data
   */
  virtual bool DataReceived(std::span<const std::byte> s) noexcept = 0;
};
