// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The XCSoar Project

#include "util/HexString.hpp"
#include "TestUtil.hpp"

#include <stdio.h>

int main()
{
  plan_tests(1);

  std::array<std::byte, 32> expected = {
    std::byte{0x9e}, std::byte{0x52}, std::byte{0x22}, std::byte{0x98},
    std::byte{0xb0}, std::byte{0x98}, std::byte{0xb1}, std::byte{0xb7},
    std::byte{0x8c}, std::byte{0x54}, std::byte{0xb5}, std::byte{0x21},
    std::byte{0xaa}, std::byte{0xec}, std::byte{0x96}, std::byte{0xba},
    std::byte{0xef}, std::byte{0xe3}, std::byte{0x79}, std::byte{0xd8},
    std::byte{0xb2}, std::byte{0x44}, std::byte{0xf9}, std::byte{0xda},
    std::byte{0x38}, std::byte{0x35}, std::byte{0xdf}, std::byte{0xe1},
    std::byte{0xf8}, std::byte{0xb6}, std::byte{0xa1}, std::byte{0x02},
  };

  std::array<std::byte, 32> raw_hash;
  const std::string_view hex_str = std::string_view(
    "9e522298b098b1b78c54b521aaec96baefe379d8b244f9da3835dfe1f8b6a102");

  raw_hash = ParseHexString<32>(hex_str);

  ok1(raw_hash == expected);

  return exit_status();
}
