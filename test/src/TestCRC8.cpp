// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The XCSoar Project

#include "util/CRC8.hpp"

extern "C" {
#include "tap.h"
}

int main()
{
  plan_tests(2);

  uint8_t msg[] = {0x01,0x02,0x03,0x04,0x05,0x06,0x07,0x08,0x09};
  auto crc = UpdateCRC8(std::as_bytes(std::span{msg}), std::byte{0xFF});
  ok1(crc == std::byte{0x6A});

  crc = UpdateCRC8(std::byte{0x01},std::byte{0xFF});
  crc = UpdateCRC8(std::byte{0x02},crc);
  crc = UpdateCRC8(std::byte{0x03},crc);
  ok1(crc == std::byte{0x6C});

  return exit_status();
}
