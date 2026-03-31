// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The XCSoar Project

#pragma once

#include "Device/Driver.hpp"

#include <cstdint>
#include <span>
#include <vector>

struct NMEAInfo;

class GDL90Device final : public AbstractDevice {
  std::vector<uint8_t> buffer;
  std::vector<uint8_t> unescaped;

  void ParseTrafficReport(std::span<const uint8_t> payload,
                          NMEAInfo &info) noexcept;

  void ParseOwnshipReport(std::span<const uint8_t> payload,
                          NMEAInfo &info) noexcept;

public:
  bool DataReceived(std::span<const std::byte> s,
                    NMEAInfo &info) noexcept override;
};
