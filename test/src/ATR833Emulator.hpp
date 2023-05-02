// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The XCSoar Project

#pragma once

#include "DeviceEmulator.hpp"
#include "io/DataHandler.hpp"
#include "util/StaticFifoBuffer.hxx"
#include "RadioFrequency.hpp"

#include <cstddef> // for std::byte

class ATR833Emulator final : public DeviceEmulator, DataHandler {
  StaticFifoBuffer<std::byte, 256u> buffer;

  RadioFrequency active_frequency = RadioFrequency::FromMegaKiloHertz(123, 450);
  RadioFrequency standby_frequency = RadioFrequency::FromMegaKiloHertz(121, 500);

public:
  explicit ATR833Emulator() noexcept {
    handler = this;
  }

private:
  std::size_t Handle(std::span<const std::byte> src) noexcept;

protected:
  bool DataReceived(std::span<const std::byte> s) noexcept override;
};
