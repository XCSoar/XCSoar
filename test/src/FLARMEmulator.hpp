// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The XCSoar Project

#pragma once

#include "DeviceEmulator.hpp"
#include "Device/Util/LineSplitter.hpp"
#include "util/StaticFifoBuffer.hxx"

#include <string>
#include <map>

class NMEAInputLine;

class FLARMEmulator : public DeviceEmulator, PortLineSplitter {
  std::map<std::string, std::string, std::less<>> settings;

  StaticFifoBuffer<std::byte, 256u> binary_buffer;

  bool binary = false;

public:
  FLARMEmulator() noexcept {
    handler = this;
  }

private:
  void PFLAC_S(NMEAInputLine &line) noexcept;
  void PFLAC_R(NMEAInputLine &line) noexcept;
  void PFLAC(NMEAInputLine &line) noexcept;
  void PFLAX() noexcept;

  size_t Unescape(const std::byte *const data, const std::byte *const end,
                  void *_dest, size_t length) noexcept;

  void SendACK(uint16_t sequence_number) noexcept;

  size_t HandleBinary(const void *_data, size_t length) noexcept;

  void BinaryReceived(std::span<const std::byte> s) noexcept;

protected:
  bool DataReceived(std::span<const std::byte> s) noexcept override;
  bool LineReceived(const char *_line) noexcept override;
};
